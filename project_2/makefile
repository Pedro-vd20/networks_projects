all: server client

client: client.o threading.o commands.o data_transfer.o
	gcc -pthread client.o threading.o commands.o  data_transfer.o -o client/client

client.o: client.c constants.h 
	gcc -c -pthread client.c

server: server.o threading.o commands.o data_transfer.o
	gcc -pthread server.o threading.o commands.o data_transfer.o -o server/server

server.o: server.c user.h constants.h
	gcc -c -pthread server.c

threading.o: threading.c threading.h
	gcc -c -pthread threading.c

commands.o: commands.h commands.c
	gcc -c commands.c

data_transfer.o: data_transfer.h data_transfer.c
	gcc -c data_transfer.c

clean:
	rm *.o server/server client/client
