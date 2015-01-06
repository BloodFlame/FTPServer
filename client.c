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
	int clientfd;
	struct sockaddr_in serveraddr;
	int addrlen;
	char* savepath;
	char* filename;
}Arg;

fBlock *full, *empty;
int ack, block_num;

fBlock* initblock(int maxsize)
{
	int i;
	fBlock *p, *q;
	p = (fBlock*)malloc(sizeof(fBlock)*maxsize);
	full = p;
	empty = p;

	if( p <= 0)
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

int filecreate(const char path[])
{
	int fid;
	fid = open(path, O_WRONLY|O_CREAT|O_TRUNC);
	return fid;
}

int filewrite(const int fid, char block[])
{
	int write_num = 0;
	write_num = write(fid, block, 512);
	if(write_num == 0)
	{
		printf("write block error\n");
		return 0;
	}
	return write_num;
}

void blockrecv(void* parg)
{
	Arg* arg = (Arg*)parg;
	int recv_num;
	int id = 1;
	char buf[516];
	while(1)
	{
		if(id == block_num) break;
		while(empty->next == full);
		memset(buf, 0, 516);
		recvfrom(arg->clientfd, buf, 516, 0, 
				(struct sockaddr*)&arg->serveraddr, &arg->addrlen);
		if(id == *((int*)buf))
		{
			strncpy(empty->buf, buf+4, 512);
			memset(buf, 0, 516);
			sprintf(buf,"%d", id);
			sendto(arg->clientfd, buf, 128, 0, 
					(struct sockaddr*)&arg->serveraddr, arg->addrlen);
			empty->id = id;
			empty = empty->next;
			id++;
		}
		else
		{
			memset(buf, 0, 516);
			sprintf(buf, "%d", id-1);
			sendto(arg->clientfd, buf, 128, 0, 
					(struct sockaddr*)&arg->serveraddr, arg->addrlen);
		}
	}
	return;
}

void fw(void* pfid)
{
	int fid = (int)pfid;
	int id = 1;
	while(1)
	{
		if( id == block_num+1) break;
		while(full == empty);
		filewrite( fid, full->buf);
	}
	return;
}

void sendACK(void* parg)
{
	Arg* arg = (Arg*) parg;
	fBlock *fblock;
	pthread_t pid1, pid2;
	int fid;
	char buf[128];
	unsigned long filesize;
	
	int send_num = sendto(arg->clientfd, arg->filename, 128, 0, 
			(struct sockaddr*)&arg->serveraddr, sizeof(arg->serveraddr));
	printf("%d\n", send_num);
	printf("recvfrom %s \n", inet_ntoa(arg->serveraddr.sin_addr));
	//memset(buf, 0, 128);
	printf("%d\n", arg->addrlen);

	recvfrom(arg->clientfd, &filesize, sizeof(unsigned long), 0, 
			(struct sockaddr*)&arg->serveraddr, &arg->addrlen);
	//recvfrom(arg->clientfd, buf, 128, 0, NULL, NULL);
	if( strcmp(buf, "error") == 0)
	{
		printf("%s\n",buf);
		return;
	}

	printf("The size of file \"%s\" is %lu bytes\n", arg->filename, filesize);
	block_num = (filesize%512 == 0)?(filesize/512):(filesize/512 + 1);
	
	fblock = initblock(11);

	if(pthread_create(&pid1, NULL, (void *)fw, (void*)fid) != 0 )
	{
		printf("Pthread_create fwrite error\n");
		return;
	}

	if(pthread_create(&pid2, NULL, (void *)blockrecv, (void*)arg) != 0)
	{
		printf("Pthread_create blockrecv error\n");
		return;
	}
	
	pthread_join(pid1, NULL);
	pthread_join(pid2, NULL);
	close(fid);
	free(fblock);
	printf("Receive cmplete!\n");
	return;

}

int main(int argc, char** argv)
{
	//struct sockaddr_in clientaddr;
	int clientfd;
	pthread_t pid;
	char serverip[50] = "127.0.0.1";
	char defaultname[128] = "./";
	
	if(argc < 2)
	{
		printf("Please input the target filename\n");
		return 0;
	}
	else if(argc < 3)
	{
		strcat(defaultname, argv[1]);
		argv[2] = defaultname;
	}
	
	clientfd = socket(PF_INET, SOCK_DGRAM, 0);
	Arg arg;
	arg.clientfd = clientfd;
	arg.savepath = argv[2];
	arg.filename = argv[1];
	bzero(&arg.serveraddr, sizeof(struct sockaddr_in));
	//bzero(&clientaddr, sizeof(struct sockaddr_in));
	arg.serveraddr.sin_family = AF_INET;
	arg.serveraddr.sin_port = htons(SERVER_PORT);
	arg.serveraddr.sin_addr.s_addr = inet_addr(serverip);
	arg.addrlen = sizeof(struct sockaddr_in);

	//clientaddr.sin_family =AF_INET;
	//clientaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//clientaddr.sin_port = htons(6001);

	//if(bind(clientfd, (struct sockaddr*)&clientaddr, 
	//			sizeof(struct sockaddr_in)) < 0)
	//{
	//	printf("bind clientfd error\n");
	//	return 0;
	//}
	 
	if(pthread_create(&pid, NULL, (void*)sendACK, (void*)&arg) != 0)
	{
		printf("Pthread_create sendACK error\n");
		return -1;
	}
	
	pthread_join(pid, NULL);
	close(clientfd);
	
	return 0;
}
