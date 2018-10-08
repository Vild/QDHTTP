#ifndef CLIENT_H_
#define CLIENT_H_

#include <time.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include <qdhttp/string.h>

struct Client {
	int fd;
	bool dead;
	string request;
	time_t connectAt;
	struct sockaddr_in addr;

	string webRoot;
};

struct Client* client_init(int fd, time_t currentTime, struct sockaddr_in addr, string webRoot);
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

#endif
