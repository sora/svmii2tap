TARGET = tapdev

CC = gcc
CFLAGS = -Wall -O

all: $(TARGET)

clean:
	rm -f  *.o $(TARGET)

.PHONY: clean
