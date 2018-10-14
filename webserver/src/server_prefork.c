#include <qdhttp/server_prefork.h>

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
#include <stddef.h>
#include <stdint.h>

static void server_prefork_init(struct Server* server);
static void server_prefork_free(struct Server* server);
static void server_prefork_handleRequests(struct Server* server);

struct ServerPreforkHandler {
	struct ServerHandler handler;
	size_t threadCount;
	struct Thread* threads;
};

static struct ServerPreforkHandler serverPreforkHandler = {
	{
		init: &server_prefork_init,
		free: &server_prefork_free,
		handleRequests: &server_prefork_handleRequests,
	},
	0, NULL
};

struct ThreadInfo {
	int clientFD;
	struct sockaddr_in addr;
	struct Server* server;
	struct Client* client;
};

struct Thread {
	bool die;
	pthread_t thread;
	struct ThreadInfo* threadInfo;
	pthread_mutex_t waitForClient;
};

struct ServerHandler* server_prefork_getHandler(void) {
	return &serverPreforkHandler.handler;
}


static void* _clientHandle(void* arg);

static void server_prefork_init(struct Server* server) {
	int flags = fcntl(server->fd, F_GETFL, 0);
	if (flags < 0) {
		perror("fcntl: F_GETFL");
		exit(EXIT_FAILURE);
	}
	if (fcntl(server->fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		perror("fcntl: F_SETFL");
		exit(EXIT_FAILURE);
	}

	serverPreforkHandler.threadCount = 16; //TODO: Read from config
	serverPreforkHandler.threads = malloc(sizeof(struct Thread) * serverPreforkHandler.threadCount);
	memset(serverPreforkHandler.threads, 0, sizeof(struct Thread) * serverPreforkHandler.threadCount);

	for (size_t i = 0; i < serverPreforkHandler.threadCount; i++) {
		if (pthread_mutex_init(&serverPreforkHandler.threads[i].waitForClient, NULL)) {
			perror("pthread_mutex_init");
			exit(EXIT_FAILURE);
		}
		if (pthread_create(&serverPreforkHandler.threads[i].thread, NULL, &_clientHandle, &serverPreforkHandler.threads[i])) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}
}

static void server_prefork_free(struct Server* server) {
	(void)server;

	for (size_t i = 0; i < serverPreforkHandler.threadCount; i++) {
		struct Thread* t = &serverPreforkHandler.threads[i];
		t->die = true;
		pthread_mutex_unlock(&t->waitForClient);
		//pthread_mutex_destroy(&t->waitForClient);
		//pthread_kill(t->thread, SIGTERM);
		pthread_cancel(t->thread);
		pthread_join(t->thread, NULL);

		if (t->threadInfo) {
			client_free(t->threadInfo->client);
			free(t->threadInfo);
		}
	}

	free(serverPreforkHandler.threads);
}


static void* _clientHandle(void* arg) {
	struct Thread* t = arg;
	pthread_mutex_lock(&t->waitForClient);
	fprintf(stderr, "[%p] Waiting\n", t);
	// Will sleep until a client has been set
	// It will be unlocked from the _clientRequest function
	while (pthread_mutex_lock(&t->waitForClient), true) {
		if (t->die)
			return NULL;
		fprintf(stderr, "[%p] Got client %d\n", t, t->threadInfo->clientFD);
		while (t->threadInfo && t->threadInfo->client && !t->threadInfo->client->dead) {
			size_t offset = string_getSize(t->threadInfo->client->request);
			ssize_t amount = read(t->threadInfo->client->fd, t->threadInfo->client->request + offset, string_getSpaceLeft(t->threadInfo->client->request));
			if (amount <= 0 && errno != EAGAIN) {
				t->threadInfo->client->dead = true;
				break;
			}

			string_setSize(t->threadInfo->client->request, offset + (size_t)amount);
			client_update(t->threadInfo->client, time(NULL));
		}
		fprintf(stderr, "[%p] Client done\n", t);
	}
	return NULL;
}

static void server_prefork_handleRequests(struct Server* server) {
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
		ti->addr = addr;
		ti->clientFD = clientFD;
		ti->server = server;
		ti->client = client_init(clientFD, time(NULL), ti->addr, server->webRoot);

		fprintf(stderr, "Searching for thread...\n");
		bool foundPlace = false;
		while (!foundPlace) {
			for (size_t i = 0; i < serverPreforkHandler.threadCount; i++) {
				struct Thread* t = &serverPreforkHandler.threads[i];
				if (!t->threadInfo) {
					foundPlace = true;
					t->threadInfo = ti;
					pthread_mutex_unlock(&t->waitForClient);
					break;
				}
			}
		}
	}

	// Reap old clients
	for (size_t i = 0; i < serverPreforkHandler.threadCount; i++) {
		struct Thread* t = &serverPreforkHandler.threads[i];

		if (t->threadInfo && t->threadInfo->client->dead) {
			struct ThreadInfo* ti = t->threadInfo;
			t->threadInfo = NULL;

			client_free(ti->client);
			free(ti);
		}
	}
}
