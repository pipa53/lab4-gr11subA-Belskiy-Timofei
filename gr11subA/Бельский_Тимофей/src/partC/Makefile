CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = page_faults

all: $(TARGET)

$(TARGET): page_faults.c
	$(CC) $(CFLAGS) -o $(TARGET) page_faults.c

clean:
	rm -f $(TARGET)

run:
	./$(TARGET)
