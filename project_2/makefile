all: server client

client: client.o threading.o commands.o
	gcc -pthread client.o threading.o commands.o -o client/client

client.o: client.c
	gcc -c -pthread client.c

server: server.o threading.o commands.o
	gcc -pthread server.o threading.o commands.o -o server/server

server.o: server.c user.h
	gcc -c -pthread server.c

threading.o: threading.c threading.h
	gcc -c -pthread threading.c

commands.o: commands.h commands.c
	gcc -c commands.c

clean:
	rm *.o server/server client/client
