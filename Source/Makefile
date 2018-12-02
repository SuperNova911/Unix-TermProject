CC = gcc
CFLAGS = -W -Wall -std=c99 -lncursesw -I/usr/include/mysql -L/usr/lib/mysql -lmysqlclient
OBJECT1 = client.o dataPack.o interface.o user.o lecture.o
OBJECT2 = server.o dataPack.o interface.o database.o user.o lecture.o

all : client server

client : $(OBJECT1)
	$(CC) -o client $(OBJECT1) $(CFLAGS)
	
server : $(OBJECT2)
	$(CC) -o server $(OBJECT2) $(CFLAGS)

clean :
	rm *.o