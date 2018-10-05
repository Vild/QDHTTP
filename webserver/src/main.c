#include <stdio.h>

#include <qdhttp/string.h>
#include <qdhttp/socket.h>

#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>

struct Server* server = NULL;
static void onSignal(int sig) {
	if (sig == SIGINT)
		printf("\nCaught SIGINT. Terminating...\n");
	else if (sig == SIGINT)
		printf("\nCaught SIGINT. Terminating...\n");
	server_free(server);
	exit(0);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

  struct sigaction sa;
	sa.sa_handler = &onSignal;
	if (sigaction(SIGINT, &sa, 0) != 0) {
    perror("sigaction()");
    return -1;
  }
  if (sigaction(SIGPIPE, &sa, 0) != 0) {
    perror("sigaction()");
    return -1;
	}

	// TODO: Read from argv
	string ip = string_initFromCStr("0.0.0.0");
	uint16_t port = 8888;
	size_t maxAmountOfClient = 64;

	server = server_init(ip, port, maxAmountOfClient);
	while (true) {
		server_freeDeadClients(server);
		server_aquireNewClients(server);
		server_handleRequests(server);
	}
	server_free(server);

	return 0;
}
