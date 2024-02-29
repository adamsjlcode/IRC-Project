CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS =

# Source files
SERVER_SOURCE = chatsev.c
CLIENT_SOURCE = chatcli.c

# Object files
SERVER_OBJECT = $(SERVER_SOURCE:.c=.o)
CLIENT_OBJECT = $(CLIENT_SOURCE:.c=.o)

# Executables
SERVER_EXECUTABLE = chatser
CLIENT_EXECUTABLE = chatcli

all: $(SERVER_EXECUTABLE) $(CLIENT_EXECUTABLE)

$(SERVER_EXECUTABLE): $(SERVER_OBJECT)
	$(CC) $(LDFLAGS) -o $@ $^

$(CLIENT_EXECUTABLE): $(CLIENT_OBJECT)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(SERVER_EXECUTABLE) $(CLIENT_EXECUTABLE) $(SERVER_OBJECT) $(CLIENT_OBJECT)
