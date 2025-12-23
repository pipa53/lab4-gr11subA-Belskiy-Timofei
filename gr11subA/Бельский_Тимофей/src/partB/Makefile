CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = file_analyzer

all: $(TARGET)

$(TARGET): file_analyzer.c
	$(CC) $(CFLAGS) -o $(TARGET) file_analyzer.c

clean:
	rm -f $(TARGET)

run:
	./$(TARGET)
