#include "ft.h"

block_buf* bp;
int acknum = 1;
void fload(void* pfid)
{
	int fid = (int)pfid;
	int read_num;
	int id = 1;
	while(1)
	{
		P(&bp->blocks);
		read_num = fileread(fid, id, bp->empty->packet.buf);
		bp->empty->packet.size = read_num;
		if(read_num == 0) break;
		P(&bp->mutex_e);
		bp->empty->packet.id = id;
		bp->empty = bp->empty->next;
		V(&bp->mutex_e);
		V(&bp->packets);
		id++;
	}
	return;
}

void blocksend(void* parg)
{
	Arg* arg = (Arg*)parg;
	int send_num;
	fBlock *p;
	int error_t = 0;
	while(1)
	{
		P(&bp->packets);
		P(&bp->ack);
		acknum--;
		printf("acknum is %d\n", acknum);
		P(&bp->mutex_s);
		p = bp->sending;
		V(&bp->mutex_s);
		send_num = sendto(arg->serverfd, &p->packet, sizeof(p->packet), 0, (struct sockaddr*)&arg->clientaddr, arg->addrlen);
		if(send_num > 0)
		{
			error_t = 0;
			printf("Send block [%d] success\n", p->packet.id);
			P(&bp->mutex_s);
			bp->sending = bp->sending->next;
			V(&bp->mutex_s);
		}
		else
		{
			error_t++;
			printf("Sending [%d] error!\n", p->packet.id);
			if(error_t < MAXBLOCKTIME) 
			{
				V(&bp->packets);
				continue;
			}
			else 
			{
				printf("Sending [%d] overtime.\n", p->packet.id);
				return;
			}
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
	int time_s,time_e;
	char buf[128];
	char filename[128];
	unsigned long filesize;

	memset(buf, 0, 128);
	memset(filename, 0, 128);

	recv_num = recvfrom(arg->serverfd, buf, 128, 0, 
			(struct sockaddr*)&arg->clientaddr, &arg->addrlen);

	printf("recv %d from %s is %s\n", recv_num, 
			(char*)inet_ntoa(arg->clientaddr.sin_addr), buf);

	sprintf(filename,"./%s",buf);
	fid = fileopen(filename);
	
	while(fid < 0)
	{
		if(send_num = sendto(arg->serverfd, "error", 5, 0, 
				(struct sockaddr*)&arg->clientaddr, arg->addrlen) <= 0)
		{
			printf("sendto error\n");
			return;
		}
		printf("sending error message success , total%d bytes.\n", send_num);
		memset(buf, 0, 128);
		recvfrom(arg->serverfd, buf, 128, 0, 
				(struct sockaddr*)&arg->clientaddr, &arg->addrlen);
		sprintf(filename, "./%s",buf);
		fid = fileopen(filename);
	}

	filesize = get_file_size(filename);
	printf("send filesize %lu\n", filesize);
	sendto(arg->serverfd, &filesize, sizeof(unsigned long), 0, 
			(struct sockaddr*)&arg->clientaddr, arg->addrlen);
	
	initblock(bp, BLOCK_BUF_SIZE);
	bp->total = (filesize%512 == 0)?(filesize/512):(filesize/512 + 1);
	time_s = (int)clock();
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
		//memset(buf, 0, 128);
		int ack;
		recvfrom(arg->serverfd, &ack, 4, 0, 
				(struct sockaddr*)&arg->clientaddr, &arg->addrlen);
		//How to resend		
		printf("receive ack [%d]\n", ack);
		printf("buf is 0x%x\n", ack);
		if (bp->full->packet.id == ack)
		{
			if( ack == bp->total)
			{
				break;
			}
			while( bp->full->packet.id == ack)
			{
				memset(bp->full->packet.buf, 0, sizeof(bp->full->packet.buf));
				P(&bp->mutex_f);
				bp->full->packet.id = 0;
				bp->full = bp->full->next;
				V(&bp->mutex_f);
				V(&bp->blocks);
			}
			
			acknum++;
			printf("acknum is %d\n", acknum);
			V(&bp->ack);
		}
		else
		{
			//fBlock *p, *q;
			//q = bp->full;
			P(&bp->mutex_s);
			//p = bp->sending;
			bp->sending = bp->full;
			V(&bp->mutex_s);
			//while(q != p) 
			//{
				V(&bp->packets);
				//q = q->next;
			//}
			V(&bp->ack);
		}
	}
	time_e = (int)clock();
	close(fid);
	freeblock(bp);
	printf("Transelate complete!\n");
	printf("Total cost:%d ms.\n", time_e-time_s);
	return;
}

int main(int argc, char** argv)
{
	pthread_t pid;
	Arg arg;
	block_buf blockbuf;

	bp = &blockbuf;
	initsocket(&arg);

	bind(arg.serverfd, (struct sockaddr*)&arg.serveraddr, arg.addrlen);
	if(pthread_create(&pid, NULL, (void*)recvACK, (void*)&arg) != 0)
	{
		printf("Pthread_create recvACK error\n");
		return -1;
	}
	pthread_join(pid, NULL);
	close(arg.serverfd);
	return 0;
}
