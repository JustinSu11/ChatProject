CC     = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -pthread

# Source files for each binary.
# The server links the registry, router, and logger modules.
# The client only needs utils; routing logic lives on the server.
SERVER_SRC = server.c utils.c registry.c router.c logger.c
CLIENT_SRC = client.c utils.c

SERVER = server
CLIENT = client

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC)

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC)

run-server: $(SERVER)
	./$(SERVER)

run-client: $(CLIENT)
	./$(CLIENT)

# Debug builds: no optimisation, include debug symbols for use with gdb.
debug-server: $(SERVER_SRC)
	$(CC) -g -O0 -Wall -std=c11 -pthread -o $(SERVER) $(SERVER_SRC)

debug-client: $(CLIENT_SRC)
	$(CC) -g -O0 -Wall -std=c11 -pthread -o $(CLIENT) $(CLIENT_SRC)

clean:
	rm -f $(SERVER) $(CLIENT)

.PHONY: all run-server run-client debug-server debug-client clean
