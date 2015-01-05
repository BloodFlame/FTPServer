#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<pthread.h>

#define SERVER_PORT 6000

typedef struct Block
{ 
	char buf[516];
	fBlock* next;
	int id;
}fBlock;

fBlock* full,empty,sending;

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
	serverfd = socket(PF_INET, SOCK_DGRAM, 0);
	bzero(&serveraddr, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVER_PORT);
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

void fload(int fid)
{
	int read_num;
	int id = 1;
	while(1)
	{
		while(empty->next == full);
		read_num = fileread(fid, id++, empty->buf);
		if(read_num == 0) break;
		empty->id = id-1;
		empty = empty->next;
	}
	return;
	
}

void blocksend(void* arg)
{
	int send_num;
	int i = 0;
	while(1)
	{
		while(sending == empty);
		send_num = sendto(arg->serverfd, sending->buf, sizeof(sending->buf), 0, (struct sockaddr*)&arg->clientaddr, &arg->addrlen);
		if(send_num > 0)
		{
			printf("Send block [%d] success\n", sending->id);
			i++;
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

int recvACK(void* arg)
{
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
		sendto(arg->serverfd, buf, 128, 0, (struct sockaddr*)&arg->clientaddr, &arg->addrlen);
		memset(buf, 0, 128);
		recvfrom(arg->serverfd, buf, 128, 0, (struct sockaddr*)&arg->clientaddr, &arg->addrlen);
		sprintf(filename, "./%s",buf);
		fid = fileopen(filename);
	}

	fblock = initblock(11);

	if(pthread_create(&pid[0], NULL, fload, fid) != OK)
	{
		printf("Pthread_create fload error\n");
		return -3;
	}
	if(pthread_create(&pid[1], NULL, blocksend, arg) != OK)
	{
		printf("Pthread_create blocksend error\n");
		return -4;
	}
	memset(buf, 0, 128);
	while(recvfrom(arg->serverfd, buf, 128, 0, (struct sockaddr*)&arg->clientaddr, &arg->addrlen))
	{
		//How to resend		
	}
	
}

int main(int argc, char** argv)
{
	struct sockaddr_in serveraddr;
	int serverfd, fid;
	fBlock* fblock;
	initsocket(&serveraddr, &serverfd);
	bind(serverfd, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
	if(pthread_create(&pid[0], NULL, recvACK, &arg) != OK)
	{
		printf("Pthread_create recvACK error\n");
		return -2;
	}

	return 0;

}
