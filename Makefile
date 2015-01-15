all:
	gcc -g ./ftserver.c ./ft.c -o ftserver -lpthread
	gcc -g ./client.c ./ft.c -o client -lpthread
