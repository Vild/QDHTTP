#ifndef SOCKET_H_
#define SOCKET_H_

#include <qdhttp/helper.h>
#include <qdhttp/string.h>

#include <stdint.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

#define dropClientAfter 2 /* Seconds */

struct Client {
	int fd;
	bool dead;
	string request;
	time_t connectAt;
	struct sockaddr_in addr;
};

struct Server {
	int fd;
	fd_set readfds;

  struct Client** clients;
  struct Client** clientsOther; ///< Used when removing items
	size_t clientCount;
	size_t clientCapacity;
};

struct Client* client_init(int fd, time_t currentTime, struct sockaddr_in addr);
void client_free(struct Client* client);

/**
 * This one will:
 *	1. Check if a request has been gotten
 *		1.1 Handle that request
 *		1.2 Mark as dead
 *	2. Or, check if the client has been inactive for a while
 *		2.1 Mark it as dead
 */
void client_update(struct Client* client, time_t currentTime);

struct Server* server_init(string ip, uint16_t port, size_t clientCapacity);
void server_free(struct Server* server);

void server_freeDeadClients(struct Server* server);
void server_aquireNewClients(struct Server* server);
void server_handleRequests(struct Server* server);

#endif
