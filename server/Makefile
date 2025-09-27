CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror

TARGET := aesdsocket

all: $(TARGET)

$(TARGET): aesdsocket.o
	$(CC) $(CFLAGS) -o $(TARGET) aesdsocket.o

aesdsocket.o: aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c


clean:
	rm -f $(TARGET) *.o
