CC=gcc
CFLAGS=-g -Wall

INCLUDE=-I./

LIBS= -lm

TARGET=portmap

all:net.o log.o event.o main.o
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) $^ -o $(TARGET)

%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $<  -o $@

clean:
	rm -rf *.o portmap
