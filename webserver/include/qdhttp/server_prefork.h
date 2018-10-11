#ifndef SERVER_PREFORK_H
#define SERVER_PREFORK_H

#include <qdhttp/server.h>

struct ServerHandler* server_prefork_getHandler(void);

#endif
