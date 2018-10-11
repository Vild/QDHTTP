#include <qdhttp/server_fork.h>

#include <qdhttp/client.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

static void server_fork_init(struct Server* server);
static void server_fork_free(struct Server* server);
static void server_fork_handleRequests(struct Server* server);

struct ServerForkHandler {
	struct ServerHandler handler;
};

static struct ServerForkHandler serverForkHandler = {
	{
		init: &server_fork_init,
		free: &server_fork_free,
		handleRequests: &server_fork_handleRequests,
	}
};

struct ServerHandler* server_fork_getHandler(void) {
	return &serverForkHandler.handler;
}

static void server_fork_init(struct Server* server) {
	(void)server;
}

static void server_fork_free(struct Server* server) {
	(void)server;
}

static void onSignal(int sig) {
	(void)sig;
	exit(0);
}

static void server_fork_handleRequests(struct Server* server) {
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	int clientFD = accept(server->fd, (struct sockaddr*)&addr, &addrlen);
	if (clientFD == -1) {
		perror("accept");
		exit(EXIT_FAILURE);
	}

	int pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if (pid) {
		close(clientFD);
		return;
	}

  struct sigaction sa;
	sa.sa_handler = &onSignal;
	if (sigaction(SIGINT, &sa, 0) != 0) {
    perror("sigaction()");
		exit(EXIT_FAILURE);
  }
  if (sigaction(SIGPIPE, &sa, 0) != 0) {
    perror("sigaction()");
		exit(EXIT_FAILURE);
	}

	time_t currentTime = time(NULL);
	struct Client* client = client_init(clientFD, time(NULL), addr, server->webRoot);
	while (!client->dead) {
		size_t offset = string_getSize(client->request);
		ssize_t amount = read(client->fd, client->request + offset, string_getSpaceLeft(client->request));
		if (amount < 0) {
			client->dead = true;
			break;
		}
		string_setSize(client->request, offset + (size_t)amount);
		client_update(client, currentTime);
		if (amount == 0) {
			client->dead = true;
			break;
		}
	}
	client_free(client);
	exit(0);
}
