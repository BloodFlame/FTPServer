#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<pthread.h>
#include<semaphore.h>

#define SERVER_PORT 6000
#define BUFFERSIZE 512

typedef struct Packet
{
	char buf[BUFFERSIZE];
	int id;
}fPacket;

typedef struct Block
{
	fPacket packet;
	struct Block *next;
}fBlock;

typedef struct Block_buf
{
	fBlock *block;
	int total;
	int maxsize;
	fBlock *full;
	fBlock *empty;
	fBlock *sending;
	sem_t mutex;
	sem_t blocks;   /*available blocks*/
	sem_t packets;  /*available packets*/
}block_buf;

typedef struct arguments
{
	int serverfd;
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;
	int addrlen;
}Arg;

void* P = sem_wait;
void* V = sem_post;

