CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS =

# Source files
SERVER_SOURCE = src/chatsev.c
CLIENT_SOURCE = src/chatcli.c

# Object files
SERVER_OBJECT = $(SERVER_SOURCE:.c=.o)
CLIENT_OBJECT = $(CLIENT_SOURCE:.c=.o)

# Executables
SERVER_EXECUTABLE = chatsev
CLIENT_EXECUTABLE = chatcli

# Test configuration
TEST_SOURCES = $(wildcard tests/*.c)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)
TEST_EXECUTABLE = chat_tests
TEST_LDFLAGS = -lcheck

all: $(SERVER_EXECUTABLE) $(CLIENT_EXECUTABLE)

# Main application
$(SERVER_EXECUTABLE): $(SERVER_OBJECT)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(CLIENT_EXECUTABLE): $(CLIENT_OBJECT)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Generic rule for creating object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Tests
test: $(TEST_EXECUTABLE)
	./$(TEST_EXECUTABLE)

$(TEST_EXECUTABLE): $(TEST_OBJECTS) $(SERVER_OBJECT) $(CLIENT_OBJECT)
	$(CC) $(CFLAGS) $(TEST_LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f $(SERVER_EXECUTABLE) $(CLIENT_EXECUTABLE) $(TEST_EXECUTABLE) $(SERVER_OBJECT) $(CLIENT_OBJECT) $(TEST_OBJECTS)