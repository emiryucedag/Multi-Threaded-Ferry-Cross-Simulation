CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = ferry_cross
SOURCES = ferry_cross.c

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

.PHONY: clean