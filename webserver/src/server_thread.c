#include <qdhttp/server_thread.h>

#include <qdhttp/client.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

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


struct ThreadInfo {
	struct ThreadInfo* next;
	int clientFD;
	struct sockaddr_in addr;
	struct Server* server;
	struct Client* client;
	pthread_t thread;
};

static struct ThreadInfo* first = NULL;
static struct ThreadInfo* last = NULL;

#define whereToAppend() (last ? &last->next : &last)

struct ServerHandler* server_thread_getHandler(void) {
	return &serverThreadHandler.handler;
}

static void server_thread_init(struct Server* server) {
	int flags = fcntl(server->fd, F_GETFL, 0);
	if (flags < 0) {
		perror("fcntl: F_GETFL");
		exit(EXIT_FAILURE);
	}
	if (fcntl(server->fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		perror("fcntl: F_SETFL");
		exit(EXIT_FAILURE);
	}
}

static void server_thread_free(struct Server* server) {
	(void)server;

	while (first) {
		struct ThreadInfo* ti = first;
		first = first->next;

		// If the thread is not yet dead, cancel it
		pthread_cancel(ti->thread);
		pthread_join(ti->thread, NULL);

		client_free(ti->client);
		free(ti);
	}
}


static void* _clientHandle(void* arg) {
	struct ThreadInfo* ti = arg;
	while (!ti->client->dead) {
		size_t offset = string_getSize(ti->client->request);
		ssize_t amount = read(ti->client->fd, ti->client->request + offset, string_getSpaceLeft(ti->client->request));
		if (amount <= 0 && errno != EAGAIN)
			break;

		string_setSize(ti->client->request, offset + (size_t)amount);
		client_update(ti->client, time(NULL));
	}
	ti->client->dead = true;
	return NULL;
}

static void server_thread_handleRequests(struct Server* server) {
	// Try and get new client

	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	int clientFD = accept(server->fd, (struct sockaddr*)&addr, &addrlen);
	if (clientFD != -1) {
		int flags = fcntl(clientFD, F_GETFL, 0);
		if (flags < 0) {
			perror("fcntl: F_GETFL");
			exit(EXIT_FAILURE);
		}
		if (fcntl(clientFD, F_SETFL, flags | O_NONBLOCK) < 0) {
			perror("fcntl: F_SETFL");
			exit(EXIT_FAILURE);
		}

		struct ThreadInfo* ti = malloc(sizeof(struct ThreadInfo));
		ti->next = NULL;
		ti->addr = addr;
		ti->clientFD = clientFD;
		ti->server = server;
		ti->client = client_init(clientFD, time(NULL), ti->addr, server->webRoot);

		if (pthread_create(&ti->thread, NULL, &_clientHandle, ti)) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}

		*whereToAppend() = ti;
		if (!first)
			first = ti;
		last = ti;
	}

	// Reap old clients
	while (first && first->client->dead) {
		struct ThreadInfo* ti = first;
		first = first->next;
		if (last == ti)
			last = NULL;

		// If the thread is not yet dead (this would be weird), cancel it
		pthread_cancel(ti->thread);
		pthread_join(ti->thread, NULL);

		client_free(ti->client);
		free(ti);
	}
}
