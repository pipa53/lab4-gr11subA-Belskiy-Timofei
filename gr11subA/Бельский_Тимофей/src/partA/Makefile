CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = memory_info

all: $(TARGET)

$(TARGET): memory_info.c
	$(CC) $(CFLAGS) -o $(TARGET) memory_info.c

clean:
	rm -f $(TARGET)

run:
	./$(TARGET)
