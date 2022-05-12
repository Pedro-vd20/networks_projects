all: server client

client: client.o commands.o data_transfer.o
	gcc client.o commands.o  data_transfer.o -o client/client

client.o: client.c constants.h 
	gcc -c client.c

server: server.o commands.o data_transfer.o linked_list.o
	gcc server.o commands.o data_transfer.o linked_list.o -o server/server

server.o: server.c user.h constants.h linked_list.h
	gcc -c server.c

commands.o: commands.h commands.c
	gcc -c commands.c

data_transfer.o: data_transfer.h data_transfer.c
	gcc -c data_transfer.c

linked_list.o: linked_list.h linked_list.c
	gcc -c linked_list.c

clean:
	rm *.o server/server client/client
