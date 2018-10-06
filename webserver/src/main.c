#include <qdhttp/string.h>
#include <qdhttp/socket.h>
#include <qdhttp/log.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

struct Server* server = NULL;
static void onSignal(int sig) {
	if (sig == SIGINT)
		printf("\nCaught SIGINT. Terminating...\n");
	else if (sig == SIGINT)
		printf("\nCaught SIGINT. Terminating...\n");
	if (server)
		server_free(server);
	exit(0);
}

enum Request {
	REQUEST_FORK = 0,
	REQUEST_THREAD,
	REQUEST_PREFORK,
	REQUEST_MUX,
};

int main(int argc, char** argv) {
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
	size_t maxAmountOfClient = 64;

	int isDaemon = false;
	string logFile = NULL;
	uint16_t port = 8888;
	enum Request request = REQUEST_MUX;

	(void)request;

	int showHelp = false;

	struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{"port", required_argument, NULL, 'p' },
		{"daemon", no_argument, NULL, 'd'},
		{"logfile", required_argument, NULL, 'l'},
		{"request", required_argument, NULL, 's'},
		{0, 0, 0, 0}
	};

	int c;
	while ((c = getopt_long(argc, argv, ":hp:dl:s:", longopts, NULL)) != -1) {
    switch (c) {
    case 'h':
			showHelp = true;
			break;
    case 'p':
			port = (uint16_t)atoi(optarg);
			break;
    case 'd':
			isDaemon = true;
			break;
    case 'l':
			logFile = string_initFromCStr(optarg);
			break;
    case 's':
			if (!strcmp(optarg, "fork"))
				request = REQUEST_FORK;
			else if (!strcmp(optarg, "thread"))
				request = REQUEST_THREAD;
			else if (!strcmp(optarg, "prefork"))
				request = REQUEST_PREFORK;
			else if (!strcmp(optarg, "mux"))
				request = REQUEST_MUX;
			else {
				fprintf(stderr, "%s: option `-s' only accepts [fork | thread | prefork | mux]\n", argv[0]);
				showHelp = true;
			}
			break;
    case 0:
			/* getopt_long() set a variable, just keep going */
			break;
    case ':':
			/* missing option argument */
			fprintf(stderr, "%s: option `-%c' requires an argument\n", argv[0], optopt);
			showHelp = true;
			break;
    case '?':
    default:
			/* invalid option */
			fprintf(stderr, "%s: option `-%c' is invalid: ignored\n", argv[0], optopt);
			showHelp = true;
			break;
    }
	}

	if (showHelp) {
		printf("%s:\n", argv[0]);
		printf("\t-h | --help           - Print this help.\n");
		printf("\t-p | --port <port>    - Listen to port number <port>.\n");
		printf("\t-d | --daemon         - Run as a daemon instead of as a normal program.\n");
		printf("\t-l | --logfile <file> - Log to file. If this option is not specified, logging\n");
		printf("\t                        will be only output to syslog, which is default.\n");
		printf("\t                        Files will be called <file>.log and <file>.err.\n");
		printf("\t-s | --request [fork | thread | prefork | mux]\n");
		printf("\t                      - Select request handling method.\n");
		return 0;
	}

	log_init(&logFile, isDaemon);
	string_free(logFile);

	log_error(time(NULL), "error", "client 0.0.0.0", "This is not a error!");

	server = server_init(ip, port, maxAmountOfClient);
	while (true) {
		server_freeDeadClients(server);
		server_aquireNewClients(server);
		server_handleRequests(server);
	}
	server_free(server);

	log_free();

	return 0;
}
