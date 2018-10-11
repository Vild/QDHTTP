#include <qdhttp/server_mux.h>

#include <qdhttp/client.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>

static void server_mux_init(struct Server* server);
static void server_mux_free(struct Server* server);
static void server_mux_handleRequests(struct Server* server);

struct ServerMuxHandler {
	struct ServerHandler handler;

	int epollFD;
};

static struct ServerMuxHandler serverMuxHandler = {
	{
		init: &server_mux_init,
		free: &server_mux_free,
		handleRequests: &server_mux_handleRequests,
	},
	0
};

struct ServerHandler* server_mux_getHandler(void) {
	return &serverMuxHandler.handler;
}

static void server_mux_init(struct Server* server) {
	serverMuxHandler.epollFD = epoll_create1(EPOLL_CLOEXEC);
	if (serverMuxHandler.epollFD == -1) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	struct epoll_event event;
	event.data.fd = server->fd;
	event.events = EPOLLIN;
	if (epoll_ctl(serverMuxHandler.epollFD, EPOLL_CTL_ADD, server->fd, &event) == -1) {
		perror("epoll_ctl: server->fd");
		exit(EXIT_FAILURE);
	}
}

static void server_mux_free(struct Server* server) {
	(void)server;
	close(serverMuxHandler.epollFD);
}

static void server_mux_handleRequests(struct Server* server) {
	#define MAX_EVENTS 64
	static struct epoll_event events[MAX_EVENTS];

	int nfds = epoll_wait(serverMuxHandler.epollFD, events, MAX_EVENTS, 100);
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
			if (epoll_ctl(serverMuxHandler.epollFD, EPOLL_CTL_ADD, clientFD, &ev) == -1) {
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
					if (amount < 0) {
						client->dead = true;
						break;
					}
					string_setSize(client->request, offset + (size_t)amount);					
					if (amount == 0) {
						client->dead = true;
						break;
					}
				}
			}
		}
	}

	for (size_t i = 0; i < server->clientCount; i++) {
		struct Client* client = server->clients[i];
		client_update(client, currentTime);
	}
}
