CC = gcc
CFLAGS = -W -Wall -lncursesw -std=c99
OBJECT1 = client.o dataPack.o interface.o
OBJECT2 = server.o dataPack.o interface.o database.o

all : client server

client : $(OBJECT1)
	$(CC) -o client $(OBJECT1) $(CFLAGS)
	
server : $(OBJECT2)
	$(CC) -o server $(OBJECT2) $(CFLAGS)

clean :
	rm *.o