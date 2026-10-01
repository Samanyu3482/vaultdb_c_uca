CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
CPPFLAGS = -Iinclude
TARGET = vaultdb
SRC = src/main.c src/record.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET) *.o