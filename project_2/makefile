all: server client

client: client.o threading.o
	gcc -pthread client.o threading.o -o client

client.o: client.c
	gcc -c -pthread client.c

server: server.o threading.o
	gcc -pthread server.o threading.o -o server

server.o: server.c user.h
	gcc -c -pthread server.c

threading.o: threading.c threading.h
	gcc -c -pthread threading.c

clean:
	rm *.o server client
