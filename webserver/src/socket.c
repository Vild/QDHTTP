#include <qdhttp/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

struct Client* client_init(int fd, time_t currentTime) {
	struct Client* client = malloc(sizeof(struct Client));
	client->fd = fd;
	client->dead = false;
	client->request = string_init(0x400);
	client->connectAt = currentTime;
	return client;
}

void client_free(struct Client* client) {
	close(client->fd);
	string_free(client->request);
	free(client);
}

void client_update(struct Client* client, time_t currentTime) {
	client->dead |= (currentTime - client->connectAt) >= dropClientAfter;

	if (!client->dead && string_getSize(client->request)) {
		static char output[] = "HTTP/1.0 200 OK\r\
Date: Thu, 1 Jan 70 00:00:00 GMT\r\
Server: QDHTTP\r\
Content-type: text/html\r\
\r\
<html>\
<head>\
  <title>An Example Page</title>\
</head>\
<body>\
  Hello World, this is a very simple HTML document.\
</body>\
</html>";
		(void)write(client->fd, output, sizeof(output));

		client->dead = true;
	}
}

struct Server* server_init(string ip, uint16_t port, size_t clientCapacity) {
	struct Server* server = malloc(sizeof(struct Server));

	server->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

	struct sockaddr_in address = {0};

	address.sin_family = AF_INET;
	inet_aton(ip, &address.sin_addr);
	address.sin_port = htons(port);

	(void)bind(server->fd, (struct sockaddr *)&address , sizeof(address));

	(void)listen(server->fd, clientCapacity);


	return server;
}

void server_free(struct Server* server) {
	(void)close(server->fd);
	free(server);
}

void server_freeDeadClients(struct Server* server) {
	(void)server;
}

void server_aquireNewClients(struct Server* server) {
	fd_set readfds;
	int highestFD;
	FD_ZERO(&readfds);
	FD_SET(server->fd, &readfds);
	highestFD = server->fd;

	for (size_t i = 0; i < server->clientCount; i++) {
		struct Client* client = server->clientList[i];

		FD_SET(client->fd, &readfds);
		highestFD = max(highestFD, client->fd);
	}

	int r = select(highestFD + 1, &readfds, NULL, NULL, NULL /* Wait forever */);
	if (r <= 0)
		return;

	if (FD_ISSET(server->fd, &readfds)) {
		struct sockaddr addr;
		socklen_t addrlen;
		(void)close(accept(server->fd, &addr, &addrlen));
		printf("SOMEONE CONNECT\n");
	}
}

void server_handleRequests(struct Server* server) {
	(void)server;
}
