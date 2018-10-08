#include <qdhttp/server.h>

#include <qdhttp/log.h>
#include <qdhttp/client.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

struct Server {
	int fd;
	fd_set readfds;

  struct Client** clients;
  struct Client** clientsOther; ///< Used when removing items
	size_t clientCount;
	size_t clientCapacity;

	string webRoot;
};

Server* server_init(string ip, uint16_t port, string webRoot) {
	size_t clientCapacity = 64;
	Server* server = malloc(sizeof(Server));

	server->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	int option = 1;
	setsockopt(server->fd, SOL_SOCKET , SO_REUSEADDR, (char *)&option, sizeof(option));

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

	return server;
}

void server_free(Server* server) {
	for (size_t i = 0; i < server->clientCount; i++)
		client_free(server->clients[i]);
	free(server->clients);
	free(server->clientsOther);
	close(server->fd);
	free(server);
}

void server_freeDeadClients(Server* server) {
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

void server_aquireNewClients(Server* server) {
	int highestFD = server->fd;

	FD_ZERO(&server->readfds);
	FD_SET(server->fd, &server->readfds);

	for (size_t i = 0; i < server->clientCount; i++) {
		struct Client* client = server->clients[i];

		FD_SET(client->fd, &server->readfds);
		highestFD = max(highestFD, client->fd);
	}

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 250 * 1000;

	//printf("select\n");
	int r = select(highestFD + 1, &server->readfds, NULL, NULL, &tv);
	if (r <= 0)
		return;

	//printf("server isset\n");
	if (FD_ISSET(server->fd, &server->readfds) && server->clientCount < server->clientCapacity) {
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(addr);
		int clientFD = accept(server->fd, (struct sockaddr*)&addr, &addrlen);

		printf("Client from %s:%d connect!\n", inet_ntoa(addr.sin_addr), (int)ntohs(addr.sin_port));
		if (clientFD > 0) {
			struct Client* client = client_init(clientFD, time(NULL), addr, server->webRoot);
			server->clients[server->clientCount++] = client;
		}
	}

}

void server_handleRequests(Server* server) {
	time_t currentTime = time(NULL);

	for (size_t i = 0; i < server->clientCount; i++) {
		struct Client* client = server->clients[i];
		if (FD_ISSET(client->fd, &server->readfds)) {
			ssize_t amount = read(client->fd, client->request, string_getSpaceLeft(client->request));
			if (amount <= 0)
				client->dead = true;
			printf("Read %ld bytes for client from %s:%d\n", amount, inet_ntoa(client->addr.sin_addr), (int)ntohs(client->addr.sin_port));
			string_setSize(client->request, (size_t)amount);
		}
		client_update(client, currentTime);
	}
}
