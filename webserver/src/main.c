#include <stdio.h>

#include <qdhttp/string.h>
#include <qdhttp/socket.h>

#include <stdint.h>
#include <stdbool.h>

static bool quit = false;

/*static void _onSIGINT(int i) {
	(void)i;
	quit = true;
}*/

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	// TODO: Read from argv
	string ip = string_initFromCStr("0.0.0.0");
	uint16_t port = 8888;
	size_t maxAmountOfClient = 64;

	struct Server* server = server_init(ip, port, maxAmountOfClient);
	while (!quit) {
		server_freeDeadClients(server);
		server_aquireNewClients(server);
		server_handleRequests(server);
	}
	server_free(server);

	return 0;
}
