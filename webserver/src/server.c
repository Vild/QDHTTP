#include <qdhttp/server.h>

#include <qdhttp/log.h>
#include <qdhttp/client.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct Server* server_init(struct ServerHandler* handler, string ip, uint16_t port, string webRoot) {
	size_t clientCapacity = 64;
	struct Server* server = malloc(sizeof(struct Server));

	server->fd = socket(AF_INET, SOCK_STREAM, 0);
	int option = 1;
	setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, (char *)&option, sizeof(option));

	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	inet_aton(ip, &address.sin_addr);

	if (bind(server->fd, (struct sockaddr *)&address , sizeof(address))) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(server->fd, (int)clientCapacity)) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	server->clients = malloc(sizeof(struct Client*) * clientCapacity);
	server->clientsOther = malloc(sizeof(struct Client*) * clientCapacity);
	server->clientCount = 0;
	server->clientCapacity = clientCapacity;

	server->webRoot = webRoot;

	server->handler = handler;
	server->handler->init(server);

	return server;
}

void server_free(struct Server* server) {
	server->handler->free(server);
	for (size_t i = 0; i < server->clientCount; i++)
		client_free(server->clients[i]);
	free(server->clients);
	free(server->clientsOther);
	close(server->fd);
	free(server);
}

void server_freeDeadClients(struct Server* server) {
	size_t removed = 0;
	struct Client** list = server->clientsOther;
	for (size_t i = 0; i < server->clientCount; i++) {
		struct Client* c = server->clients[i];

		if (c->dead) {
			client_free(c);
			removed++;
		} else {
			list[i - removed] = c;
		}
	}
	server->clientCount -= removed;
	server->clientsOther = server->clients;
	server->clients = list;
}

void server_handleRequests(struct Server* server) {
	server->handler->handleRequests(server);
}
