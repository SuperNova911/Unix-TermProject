CC = gcc
CFLAGS = -W -Wall -lncurses -std=c99
TARGET = client
OBJECTS = client.o dataPack.o interface.o

all : $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean :
	rm *.o