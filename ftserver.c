#include "ft.h"

block_buf* bp;


int  initblock(int maxsize)
{
	int i;
	fBlock *p,*q;
	p = (fBlock*)malloc(sizeof(fBlock)*maxsize);
	if(p <= 0)
	{
		printf("InitBlock Error!\n");
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
	sem_init(&bp->mutex, 0, 1);
	sem_init(&bp->blocks, 0, maxsize);
	sem_init(&bp->packets, 0, 0);
	return 0;
}

int initsocket(Arg *arg)
{
	arg->serverfd = socket(PF_INET, SOCK_DGRAM, 0);
	bzero(&arg->serveraddr, sizeof(struct sockaddr_in));
	arg->serveraddr.sin_family = AF_INET;
	arg->serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	arg->serveraddr.sin_port = htons(SERVER_PORT);
	arg.addrlen = sizeof(struct sockaddr_in);
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

int fileread(const int fid, const int id, char block[])
{
	char buf[BUFFERSIZE];
	int bufsize = sizeof(buf);
	int read_num = 0;
	
	memset(buf, 0, bufsize);
	read_num = read( fid, buf, bufsize);
	if (read_num < 0)
	{
		printf("Fileread %d Error", id);
		return read_num;
	}
	else if(read_num = 0)
	{
		printf("Touch the end of the file, id = %d.\n", id);
		return 0;
	}
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
		P(&bp->blocks);
		read_num = fileread(fid, id, empty->block->packet.buf);
		if(read_num == 0) break;
		bp->empty->block->packet.id = id;
		P(&bp->mutex);
		bp->empty = bp->empty->next;
		V(&bp->mutex);
		V(&bp->packets);
		id++;
	}
	total = id-1;
	return;
}

void blocksend(void* parg)
{
	Arg* arg = (Arg*)parg;
	int send_num;
	error_t = 0;
	while(1)
	{
		P(&bp->packets);
		send_num = sendto(arg->serverfd, sending->buf, sizeof(sending->buf), 0, (struct sockaddr*)&arg->clientaddr, arg->addrlen);
		if(send_num > 0)
		{
			error_t = 0;
			printf("Send block [%d] success\n", sending->id);
			P(&bp->mutex);
			sending = sending->next;
			P(&bp->mutex);
		}
		else
		{
			error_t++;
			printf("Sending [%d] error!\n", sending->id);
			if(error_t < MAXBLOCKTIME) 
			{
				V(&bp->packets);
				continue;
			}
			else return;
		}
	}
	return;
}

void recvACK(void* parg)
{
	Arg* arg = (Arg*) parg;
	pthread_t pid1, pid2; 
	int fid;
	int recv_num;
	int send_num;
	char buf[128];
	char filename[128];
	unsigned long filesize;

	memset(buf, 0, 128);
	memset(filename, 0, 128);

	recv_num = recvfrom(arg->serverfd, buf, 128, 0, 
			(struct sockaddr*)&arg->clientaddr, &arg->addrlen);

	printf("recv %d from %s is %s\n", recv_num, 
			inet_ntoa(arg->clientaddr.sin_addr), buf);

	sprintf(filename,"./%s",buf);
	fid = fileopen(filename);
	
	while(fid < 0)
	{
		memset(buf, 0, 128);
		memset(filename, 0, 128);
		sprintf(buf,"error");
		printf("%s\n",buf);
		printf("%s\n", inet_ntoa(arg->clientaddr.sin_addr));
		int send_num = sendto(arg->serverfd, "error", 5, 0, 
				(struct sockaddr*)&arg->clientaddr, sizeof(arg->clientaddr));
		printf("%d\n", send_num);
		//sendto(arg->serverfd, buf, 128, 0, NULL, NULL);
		memset(buf, 0, 128);
		printf("sendto complete %s\n",buf);
		recvfrom(arg->serverfd, buf, 128, 0, 
				(struct sockaddr*)&arg->clientaddr, &arg->addrlen);
		sprintf(filename, "./%s",buf);
		fid = fileopen(filename);
	}

	filesize = get_file_size(filename);
	//memset(buf, 0, 128);
	//sprintf(buf, "%lu", filesize);
	printf("send filesize %lu\n", filesize);
	sendto(arg->serverfd, &filesize, sizeof(unsigned long), 0, 
			(struct sockaddr*)&arg->clientaddr, sizeof(struct sockaddr_in));

	initblock(11);

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
		recvfrom(arg->serverfd, buf, 128, 0, 
				(struct sockaddr*)&arg->clientaddr, &arg->addrlen);
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
	close(fid);
	free(fblock);
	printf("Transelate complete!\n");
	return;
}

int main(int argc, char** argv)
{
	pthread_t pid;
	Arg arg;
	block_buf blockbuf;

	bp = &blockbuf;
	initsocket(&arg);

	bind(arg->serverfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if(pthread_create(&pid, NULL, (void*)recvACK, (void*)&arg) != 0)
	{
		printf("Pthread_create recvACK error\n");
		return -1;
	}
	pthread_join(pid, NULL);
	close(arg->serverfd);
	return 0;
}
