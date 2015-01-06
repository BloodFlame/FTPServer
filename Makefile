all:
	gcc -g ./ftserver.c -o ftserver -lpthread
	gcc -g ./client.c -o client -lpthread
