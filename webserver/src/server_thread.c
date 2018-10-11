#include <qdhttp/server_thread.h>

#include <qdhttp/client.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

static void server_thread_init(struct Server* server);
static void server_thread_free(struct Server* server);
static void server_thread_handleRequests(struct Server* server);

struct ServerThreadHandler {
	struct ServerHandler handler;
};

static struct ServerThreadHandler serverThreadHandler = {
	{
		init: &server_thread_init,
		free: &server_thread_free,
		handleRequests: &server_thread_handleRequests,
	}
};

struct ServerHandler* server_thread_getHandler(void) {
	return &serverThreadHandler.handler;
}

static void server_thread_init(struct Server* server) {
	(void)server;
}

static void server_thread_free(struct Server* server) {
	(void)server;
}

struct ThreadInfo {
	int clientFD;
	struct sockaddr_in addr;
	struct Server* server;
};

static void* _clientHandle(void* arg) {
	struct ThreadInfo* ti = arg;
	struct Client* client = client_init(ti->clientFD, time(NULL), ti->addr, ti->server->webRoot);
	while (!client->dead) {
		size_t offset = string_getSize(client->request);
		ssize_t amount = read(client->fd, client->request + offset, string_getSpaceLeft(client->request));
		if (amount < 0) {
			client->dead = true;
			break;
		}

		string_setSize(client->request, offset + (size_t)amount);
		client_update(client, time(NULL));

		if (amount == 0) {
			client->dead = true;
			break;
		}
	}
	client_free(client);
	free(ti);
	return NULL;
}

static void server_thread_handleRequests(struct Server* server) {
	struct ThreadInfo* ti = malloc(sizeof(struct ThreadInfo));
	socklen_t addrlen = sizeof(ti->addr);
	int clientFD = accept(server->fd, (struct sockaddr*)&ti->addr, &addrlen);
	if (clientFD == -1) {
		perror("accept");
		exit(EXIT_FAILURE);
	}
	ti->clientFD = clientFD;
	ti->server = server;

	pthread_t thread;
	if (pthread_create(&thread, NULL, &_clientHandle, ti)) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}
}
