#include "ft.h"

void P(sem_t* s)
{
	sem_wait(s);
}
void V(sem_t* s)
{
	sem_post(s);
}
int  initblock(block_buf *bp, int maxsize)
{
	int i;
	fBlock *p,*q;
	p = (fBlock*)malloc(sizeof(fBlock)*maxsize);
	if(p <= 0)
	{
		//printf("InitBlock Error!\n");
		return -1;
	}
	bp->block = p;
	bp->full = p;
	bp->empty = p;
	bp->sending = p;
	q = p;
	for(i = 0; i < maxsize-1; i++)
	{
		q->next = q+1;
		q = q->next;
	}
	q->next = p;
	bp->maxsize = maxsize;
	bp->total = 0;
	sem_init(&bp->mutex_f, 0, 1);
	sem_init(&bp->mutex_e, 0, 1);
	sem_init(&bp->mutex_s, 0, 1);
	sem_init(&bp->ack, 0, 1);
	sem_init(&bp->blocks, 0, maxsize);
	sem_init(&bp->packets, 0, 0);
	return 0;
}
void freeblock(block_buf* bp)
{
	free(bp->block);
	return;
}
int initsocket(Arg *arg)
{
	arg->serverfd = socket(PF_INET, SOCK_DGRAM, 0);
	bzero(&arg->serveraddr, sizeof(struct sockaddr_in));
	arg->serveraddr.sin_family = AF_INET;
	arg->serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	arg->serveraddr.sin_port = htons(SERVER_PORT);
	arg->addrlen = sizeof(struct sockaddr_in);
	return 0;
}

int fileopen(const char filename[])
{
	int fid;
	fid = open(filename, O_RDONLY);
	return fid;
}

unsigned long get_file_size(const char *path)
{
	unsigned long filesize = 0;
	struct stat statbuff;
	if(stat(path, &statbuff) < 0)
	{
		return filesize;
	}
	else
	{
		filesize = statbuff.st_size;
	}
	return filesize;
}

int fileread(const int fid, const int id, char buf[])
{
	int bufsize = BUFFERSIZE;
	int read_num = 0;
	int offset;
	offset = lseek(fid, 0, SEEK_CUR);
	memset(buf, 0, bufsize);
	read_num = read( fid, buf, bufsize);
	lseek(fid, BUFFERSIZE, offset); 
	if (read_num < 0)
	{
		printf("Fileread %d Error", id);
		return read_num;
	}
	else if(read_num == 0)
	{
		printf("Touch the end of the file, id = %d.\n", id);
		return 0;
	}
	//printf("Block %d is ready\n", id);
	return read_num;
}

int filecreate(const char path[])
{
	int fid;
	fid = open(path, O_RDWR|O_CREAT|O_TRUNC|O_APPEND);
	return fid;
}

int filewrite(const int fid, char block[], const int size)
{
	int write_num = 0;
	int offset;
	offset = lseek(fid, 0, SEEK_CUR);
	write_num = write(fid, block, size);
	lseek(fid , BUFFERSIZE, offset);
	if(write_num == 0)
	{
		printf("write block error\n");
		return 0;
	}
	return write_num;
}

