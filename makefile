CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS =

# Source files
SERVER_SOURCE = chatsev.c
CLIENT_SOURCE = chatcli.c

# Object files
SERVER_OBJECT = $(SERVER_SOURCE:.c=.o)
CLIENT_OBJECT = $(CLIENT_SOURCE:.c=.o)

# Executables
SERVER_EXECUTABLE = chatsev
CLIENT_EXECUTABLE = chatcli

all: $(SERVER_EXECUTABLE) $(CLIENT_EXECUTABLE)

$(SERVER_EXECUTABLE): $(SERVER_OBJECT)
    $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(CLIENT_EXECUTABLE): $(CLIENT_OBJECT)
    $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
    $(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
    rm -f $(SERVER_EXECUTABLE) $(CLIENT_EXECUTABLE) $(SERVER_OBJECT) $(CLIENT_OBJECT)
