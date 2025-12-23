CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = memory_profiler

all: $(TARGET)

$(TARGET): memory_profiler.c
	$(CC) $(CFLAGS) -o $(TARGET) memory_profiler.c

clean:
	rm -f $(TARGET)

run:
	./$(TARGET)
