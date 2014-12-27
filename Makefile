CC = gcc
CFLAGS = -Wall -O

TARGET = tapdev
OBJECTS = tapdev.o

all: $(TARGET)

clean:
	rm -f  $(TARGET) $(OBJECTS)

.PHONY: clean
