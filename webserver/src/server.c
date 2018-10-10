#include <qdhttp/server.h>

#include <qdhttp/log.h>
#include <qdhttp/client.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>

struct Server {
	int fd;
	int epollFD;

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

	server->epollFD = epoll_create1(EPOLL_CLOEXEC);
	if (server->epollFD == -1) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	struct epoll_event event;
	event.data.fd = server->fd;
	event.events = EPOLLIN;
	if (epoll_ctl(server->epollFD, EPOLL_CTL_ADD, server->fd, &event) == -1) {
		perror("epoll_ctl: server->fd");
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

void server_handleRequests(Server* server) {
	#define MAX_EVENTS 64
	static struct epoll_event events[MAX_EVENTS];

	int nfds = epoll_wait(server->epollFD, events, MAX_EVENTS, 100);
	if (nfds == -1) {
		perror("epoll_wait");
		exit(EXIT_FAILURE);
	}

	time_t currentTime = time(NULL);
	for (int i = 0; i < nfds; i++) {
		if (events[i].data.fd == server->fd && server->clientCount < server->clientCapacity) {
			struct sockaddr_in addr;
			socklen_t addrlen = sizeof(addr);
			int clientFD = accept(server->fd, (struct sockaddr*)&addr, &addrlen);

			if (clientFD == -1) {
				perror("accept");
				exit(EXIT_FAILURE);
			}
			int flags = fcntl(clientFD, F_GETFL, 0);
			if (flags < 0) {
				perror("fcntl: F_GETFL");
				exit(EXIT_FAILURE);
			}
			if (fcntl(clientFD, F_SETFL, flags | O_NONBLOCK) < 0) {
				perror("fcntl: F_SETFL");
				exit(EXIT_FAILURE);
			}

			struct epoll_event ev;
			ev.events = EPOLLIN | EPOLLET;
			ev.data.fd = clientFD;
			if (epoll_ctl(server->epollFD, EPOLL_CTL_ADD, clientFD, &ev) == -1) {
				perror("epoll_ctl: conn_sock");
				exit(EXIT_FAILURE);
			}

			if (clientFD > 0) {
				struct Client* client = client_init(clientFD, time(NULL), addr, server->webRoot);
				server->clients[server->clientCount++] = client;
			}
		} else {
			for (size_t j = 0; j < server->clientCount; j++) {
				struct Client* client = server->clients[j];
				if (events[i].data.fd == client->fd) {
					size_t offset = string_getSize(client->request);
					ssize_t amount = read(client->fd, client->request + offset, string_getSpaceLeft(client->request));
					if (amount <= 0) {
						client->dead = true;
						break;
					}
					string_setSize(client->request, offset + (size_t)amount);
					break;
				}
			}
		}
	}

	for (size_t i = 0; i < server->clientCount; i++) {
		struct Client* client = server->clients[i];
		client_update(client, currentTime);
	}
}

