CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -pedantic
TARGET  = tarsau
SRC     = tarsau.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC) tarsau.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
