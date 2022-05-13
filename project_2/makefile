all: server client

client: code/client.o code/commands.o code/data_transfer.o code/linked_list.o
	gcc code/client.o code/commands.o code/data_transfer.o code/linked_list.o -o client/client

client.o: code/client.c code/constants.h 
	gcc -c code/client.c

server: code/server.o code/commands.o code/data_transfer.o code/linked_list.o
	gcc code/server.o code/commands.o code/data_transfer.o code/linked_list.o -o server/server

server.o: code/server.c code/user.h code/constants.h code/linked_list.h
	gcc -c code/server.c

commands.o: code/commands.h code/commands.c
	gcc -c code/commands.c

data_transfer.o: code/data_transfer.h code/data_transfer.c
	gcc -c code/data_transfer.c

linked_list.o: code/linked_list.h code/linked_list.c
	gcc -c code/linked_list.c

clean:
	rm code/*.o server/server client/client
