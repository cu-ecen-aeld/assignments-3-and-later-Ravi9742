CC := $(CROSS_COMPILE)gcc

TARGET := writer
OBJS := writer.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c -o $@ $<

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
