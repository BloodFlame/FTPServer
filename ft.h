#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<pthread.h>
#include<semaphore.h>
#include<sys/time.h>

#define SERVER_PORT 6000
#define BUFFERSIZE (1024*20)
#define MAXBLOCKTIME 10
#define BLOCK_BUF_SIZE 10
#define ACK_DELAY 1

typedef struct Packet
{
	char buf[BUFFERSIZE];
	int size;
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
	sem_t mutex_f;
	sem_t mutex_e;
	sem_t mutex_s;
	sem_t ack;
	sem_t blocks;   /*available blocks*/
	sem_t packets;  /*available packets*/
}block_buf;

typedef struct arguments
{
	int serverfd;
	int clientfd;
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;
	int addrlen;
	char *filename;
	char *savepath;
}Arg;

void P(sem_t* s);
void V(sem_t* s);

int initblock(block_buf* bp, int maxsize);
void freeblock(block_buf* bp);
int initsocket(Arg *arg);
int fileopen(const char filename[]);
unsigned long get_file_size(const char *path);
int fileread(const int fid, const int id, char block[]);

int filecreate(const char path[]);
int filewrite(const int fid, char block[], const int size);
