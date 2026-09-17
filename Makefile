CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11

TARGET = vaultdb
SRC = src/main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)