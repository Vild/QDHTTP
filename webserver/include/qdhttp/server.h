#ifndef SERVER_H_
#define SERVER_H_

#include <qdhttp/string.h>
#include <stdint.h>

typedef struct Server Server;

Server* server_init(string ip, uint16_t port, string webRoot);
void server_free(Server* server);

void server_freeDeadClients(Server* server);
void server_handleRequests(Server* server);

#endif
