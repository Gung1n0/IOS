CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
LDFLAGS = -pthread -lrt
TARGET = proj2
SRC = main.c
all: $(TARGET)
$(TARGET) : $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)
clean:
	rm -f $(TARGET)