#ifndef SERVER_H_
#define SERVER_H_

#include <qdhttp/string.h>
#include <stdint.h>

struct Server;

struct ServerHandler {
	void (*init)(struct Server* server);
	void (*free)(struct Server* server);

	void (*handleRequests)(struct Server* server);
};

struct Server {
	int fd;

	struct ServerHandler* handler;

  struct Client** clients;
  struct Client** clientsOther; ///< Used when removing items
	size_t clientCount;
	size_t clientCapacity;

	string webRoot;
};


struct Server* server_init(struct ServerHandler* handler, string ip, uint16_t port, string webRoot);
void server_free(struct Server* server);

void server_freeDeadClients(struct Server* server);
void server_handleRequests(struct Server* server);

#endif
