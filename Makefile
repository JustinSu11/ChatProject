CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -pthread

SERVER_SRC = server.c utils.c
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

debug-server:
	$(CC) -g -O0 -Wall -std=c11 -pthread -o $(SERVER) $(SERVER_SRC)

debug-client:
	$(CC) -g -O0 -Wall -std=c11 -pthread -o $(CLIENT) $(CLIENT_SRC)

clean:
	rm -f $(SERVER) $(CLIENT)

.PHONY: all run-server run-client debug-server debug-client clean
