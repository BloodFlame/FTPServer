#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<pthread.h>

#define SERVER_PORT 6000

typedef struct Block
{ 
	char buf[516];
	struct Block *next;
	int id;
}fBlock;
typedef struct ARG
{
	int serverfd;
	struct sockaddr_in clientaddr;
	int addrlen;
}Arg;

fBlock *full,*empty,*sending;
int total = 0;

fBlock*  initblock(int maxsize)
{
	int i;
	fBlock *p, *q;
	p = (fBlock*)malloc(sizeof(fBlock)*maxsize);
	full = p;
	empty = p;
	sending = p;
	if(p <= 0)
	{
		printf("InitBlock Error!\n");
		return p;
	}
	q = p;
	for(i = 0; i < maxsize-1; i++)
	{
		q->next = q+1;
		q = q->next;
	}
	q->next = p;
	return p;
}

int initsocket(struct sockaddr_in *serveraddr, int *serverfd)
{
	*serverfd = socket(PF_INET, SOCK_DGRAM, 0);
	bzero(serveraddr, sizeof(struct sockaddr_in));
	serveraddr->sin_family = AF_INET;
	serveraddr->sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr->sin_port = htons(SERVER_PORT);
	return 0;
}

int fileopen(const char filename[])
{
	int fid;
	fid = open(filename, O_RDONLY);
	return fid;
}

int fileread(const int fid, const int id, char block[])
{
	char buf[512];
	int bufsize = sizeof(buf);
	int read_num = 0;
	
	memset(buf, 0, bufsize);
	read_num = read( fid, buf, bufsize);
	if (read_num < 0)
	{
		printf("Fileread %d Error", id);
		return -1;
	}
	else if(read_num = 0)
	{
		return 0;
	}
	sprintf(block,"%d%s", id, buf);
	printf("Block %d is ready\n", id);
	return read_num;
}	

void fload(void* pfid)
{
	int fid = (int)pfid;
	int read_num;
	int id = 1;
	while(1)
	{
		while(empty->next == full);
		read_num = fileread(fid, id, empty->buf);
		if(read_num == 0) break;
		empty->id = id;
		empty = empty->next;
		id++;
	}
	total = id-1;
	return;
	
}

void blocksend(void* parg)
{
	Arg* arg = (Arg*)parg;
	int send_num;
	while(1)
	{
		while(sending == empty);
		while(sending != full);
		send_num = sendto(arg->serverfd, sending->buf, sizeof(sending->buf), 0, (struct sockaddr*)&arg->clientaddr, arg->addrlen);
		if(send_num > 0)
		{
			printf("Send block [%d] success\n", sending->id);
			sending = sending->next;
		}
		else
		{
			printf("Sending [%d] error!\n", sending->id);
			continue;
		}
	}
	return;
}

void recvACK(void* parg)
{
	Arg* arg = (Arg*) parg;
	fBlock* fblock;
	pthread_t pid1, pid2; 
	int fid;
	char buf[128];
	char filename[128];
	memset(buf, 0, 128);
	memset(filename, 0, 128);
	recvfrom(arg->serverfd, buf, 128, 0, (struct sockaddr*)&arg->clientaddr, &arg->addrlen);
	sprintf(filename,"./%s",buf);
	fid = fileopen(filename);
	while(fid < 0)
	{
		memset(buf, 0, 128);
		memset(filename, 0, 128);
		sprintf(buf,"Can't translate this file\n");
		sendto(arg->serverfd, buf, 128, 0, (struct sockaddr*)&arg->clientaddr, arg->addrlen);
		memset(buf, 0, 128);
		recvfrom(arg->serverfd, buf, 128, 0, (struct sockaddr*)&arg->clientaddr, &arg->addrlen);
		sprintf(filename, "./%s",buf);
		fid = fileopen(filename);
	}

	fblock = initblock(11);

	if(pthread_create(&pid1, NULL, (void *)fload, (void *)fid) != 0)
	{
		printf("Pthread_create fload error\n");
		return;
	}
	if(pthread_create(&pid2, NULL, (void *)blocksend, (void *)arg) != 0)
	{
		printf("Pthread_create blocksend error\n");
		return;
	}

	while(1)
	{
		memset(buf, 0, 128);
		recvfrom(arg->serverfd, buf, 128, 0, (struct sockaddr*)&arg->clientaddr, &arg->addrlen);
		//How to resend		
		if (full->id == (int)*buf)
		{
			memset(full->buf, 0, sizeof(full->buf));
			if(full->id == total && total != 0)
			{
				full->id = 0;
				break;
			}
			full->id = 0;
			full = full->next;
		}
		else
		{
			sending = full;
		}
	}
	printf("Transelate complete!\n");
	return;
}

int main(int argc, char** argv)
{
	struct sockaddr_in serveraddr;
	int serverfd;
	pthread_t pid;
	Arg arg;
	initsocket(&serveraddr, &serverfd);
	arg.serverfd = serverfd;
	bind(serverfd, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
	if(pthread_create(&pid, NULL, (void*)recvACK, (void*)&arg) != 0)
	{
		printf("Pthread_create recvACK error\n");
		return -2;
	}
	pthread_join(pid, NULL);
	return 0;
}
