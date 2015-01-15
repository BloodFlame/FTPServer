#include "ft.h"

block_buf* bp;

void blockrecv(void* parg)
{
	Arg* arg = (Arg*)parg;
	int recv_num;
	int id = 1;
	int count = 1;
	int ack = 0;
	char buf[BUFFERSIZE + 12];
	fPacket* p;
	while(1)
	{
		if(id > bp->total) break;
		P(&bp->blocks);
		memset(buf, 0, sizeof(buf));
		recvfrom(arg->clientfd, buf, sizeof(fPacket) , 0, 
				(struct sockaddr*)&arg->serveraddr, &arg->addrlen);
		p = (fPacket*)buf;
		if(id == p->id)
		{
			printf("recv [%d] success.\n",id);
			memset(bp->empty->packet.buf, 0, BUFFERSIZE);
			memcpy(bp->empty->packet.buf, p->buf, BUFFERSIZE);
			bp->empty->packet.size = p->size;
			ack = id;
			P(&bp->mutex_e);
			bp->empty->packet.id = id;
			bp->empty = bp->empty->next;
			V(&bp->mutex_e);
			V(&bp->packets);
			id++;
		}
		else
		{	
			ack = id-1;
		}
		if(count == ACK_DELAY)
		{
			printf("send ack %d\n", ack);
			sendto(arg->clientfd, &ack, 4, 0, 
					(struct sockaddr*)&arg->serveraddr, arg->addrlen);
			count = 0;
		}
		count++;
	}
	return;
}

void fw(void* pfid)
{
	int fid = (int)pfid;
	int count = 0;
	while(1)
	{
		if(count >= bp->total) break;
		P(&bp->packets);
		filewrite(fid, bp->full->packet.buf, bp->full->packet.size);
		P(&bp->mutex_f);
		//memset(bp->full->packet.buf, 0, BUFFERSIZE);
		bp->full->packet.id = 0;
		bp->full = bp->full->next;
		V(&bp->mutex_f);
		V(&bp->blocks);
		printf("write %d complete\n", count+1);
		count++;
	}
	printf("RECEIV FILE COMPLETE!\n");
	return;
}

void sendACK(void* parg)
{
	Arg* arg = (Arg*) parg;
	pthread_t pid1, pid2;
	int fid;
	int send_num,recv_num;
	char buf[128];
	unsigned long filesize;

	printf("recvfrom %s \n", (char*)inet_ntoa(arg->serveraddr.sin_addr));
	if(sendto(arg->clientfd, arg->filename, 128, 0, 
			(struct sockaddr*)&arg->serveraddr, sizeof(struct sockaddr_in)) <= 0)
	{
		printf("sendto error\n");
		return;
	}

	printf("recvfrom %s \n", (char*)inet_ntoa(arg->serveraddr.sin_addr));

	if(recvfrom(arg->clientfd, &filesize, sizeof(unsigned long), 0, 
			(struct sockaddr*)&arg->serveraddr, &arg->addrlen) <= 0)
	{
		printf("recv error\n");
		return;
	}

	if( strcmp(buf, "error") == 0)
	{
		printf("%s ,can't find this file\n",buf);
		return;
	}

	printf("The size of file \"%s\" is %lu bytes\n", arg->filename, filesize);

	initblock(bp,BLOCK_BUF_SIZE);
	fid = filecreate(arg->savepath);
	bp->total = (filesize%BUFFERSIZE == 0)?(filesize/BUFFERSIZE):(filesize/BUFFERSIZE + 1);
	printf("blocks number is %d\n", bp->total);

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
	freeblock(bp);
	printf("Receive cmplete!\n");
	return;

}

int main(int argc, char** argv)
{
	Arg arg;
	block_buf blockbuf;
	bp = &blockbuf;
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

	arg.clientfd = socket(PF_INET, SOCK_DGRAM, 0);
	arg.savepath = argv[2];
	arg.filename = argv[1];
	bzero(&arg.serveraddr, sizeof(struct sockaddr_in));
	arg.serveraddr.sin_family = AF_INET;
	arg.serveraddr.sin_port = htons(SERVER_PORT);
	arg.serveraddr.sin_addr.s_addr = inet_addr(serverip);
	arg.addrlen = sizeof(struct sockaddr_in);

	if(pthread_create(&pid, NULL, (void*)sendACK, (void*)&arg) != 0)
	{
		printf("Pthread_create sendACK error\n");
		return -1;
	}
	
	pthread_join(pid, NULL);
	close(clientfd);
	
	return 0;}
