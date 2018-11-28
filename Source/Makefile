CC = gcc
CFLAGS = -W -Wall
TARGET = client
OBJECTS = client.o dataPack.o interface.o

all : $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean :
	rm *.o