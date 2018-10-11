#include <qdhttp/string.h>
#include <qdhttp/server.h>
#include <qdhttp/log.h>
#include <qdhttp/config.h>

#include <qdhttp/server_fork.h>
#include <qdhttp/server_mux.h>
#include <qdhttp/server_prefork.h>
#include <qdhttp/server_thread.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

struct Server* server = NULL;
static void onSignal(int sig) {
	if (sig == SIGINT)
		printf("\nCaught SIGINT. Terminating...\n");

	if (!server)
		server_free(server);

	kill(0, SIGINT);
	exit(0);
}

static void onChildDeath(int sig) {
	(void)sig;
	int status;
	while (true) {
		pid_t pid = waitpid(-1, &status, WNOHANG);
		if (pid <= 0)
			break;
	}
}

int main(int argc, char** argv) {
  struct sigaction sa;
	sa.sa_handler = &onSignal;
	if (sigaction(SIGINT, &sa, 0) != 0) {
    perror("sigaction()");
    return -1;
  }
	if (sigaction(SIGTERM, &sa, 0) != 0) {
    perror("sigaction()");
    return -1;
  }
  if (sigaction(SIGPIPE, &sa, 0) != 0) {
    perror("sigaction()");
    return -1;
	}
	sa.sa_handler = &onChildDeath;
  if (sigaction(SIGCHLD, &sa, 0) != 0) {
    perror("sigaction()");
    return -1;
	}

	// TODO: Read from argv
	Config* c = config_init("qdhttp-config.ini");

	uint16_t port = (uint16_t)config_getPropertyInt(c, "HTTP", "Port", 8888);
	string requestStr = config_getProperty(c, "HTTP", "Request", "mux");
	bool isRequestValueFromConfig = true;

	string logFile = config_getProperty(c, "Log", "File", NULL);
	if (logFile && string_getSize(logFile) == 0)
		logFile = NULL;

	bool isDaemon = config_getPropertyBool(c, "Server", "Daemon", false);

	int showHelp = false;

	struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{"port", required_argument, NULL, 'p' },
		{"daemon", no_argument, NULL, 'd'},
		{"logfile", required_argument, NULL, 'l'},
		{"request", required_argument, NULL, 's'},
		{0, 0, 0, 0}
	};

	{
		int opt;
		while ((opt = getopt_long(argc, argv, ":hp:dl:s:", longopts, NULL)) != -1)
			switch (opt) {
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
				requestStr = optarg;
				isRequestValueFromConfig = false;
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

	struct ServerHandler* (*getHandler)(void) = NULL;

	{
		if (!strcmp(requestStr, "fork"))
			getHandler = &server_fork_getHandler;
		else if (!strcmp(requestStr, "thread"))
			getHandler = &server_thread_getHandler;
/*		else if (!strcmp(requestStr, "prefork"))
			getHandler = &server_prefork_getHandler;*/
		else if (!strcmp(requestStr, "mux"))
			getHandler = &server_mux_getHandler;
		else {
			if (isRequestValueFromConfig)
				fprintf(stderr, "%s: Config contains invalid request handling method, it only accepts [fork | thread | prefork | mux]\n", argv[0]);
			else
				fprintf(stderr, "%s: option `-s' only accepts [fork | thread | prefork | mux]\n", argv[0]);
			showHelp = true;
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

	if (isDaemon) {
		pid_t pid = fork();
		if (pid < 0) {
			perror("fork");
			exit(EXIT_FAILURE);
		} else if (pid > 0)
			exit(EXIT_SUCCESS);

		if (setsid() < 0) {
			perror("setsid");
			exit(EXIT_FAILURE);
		}

		signal(SIGCHLD, SIG_IGN);
		signal(SIGHUP, SIG_IGN);

		pid = fork();

		if (pid < 0) {
			perror("fork");
			exit(EXIT_FAILURE);
		} else if (pid > 0) {
			fprintf(stdout, "%d\n", pid);
			exit(EXIT_SUCCESS);
		}

		umask(0);

		//TODO: chroot(dataFolder);

		for (int64_t fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--)
			close((int)fd);

		stdin = fopen("/dev/null", "r");
		stdout = fopen("/dev/null", "w+");
		stderr = fopen("/dev/null", "w+");
	}

	log_init(&logFile, isDaemon);
	string_free(logFile);

	string ip = config_getProperty(c, "HTTP", "IP", "0.0.0.0");
	string webRoot = config_getProperty(c, "HTTP", "WebRoot", "../www/");

	if (config_getPropertyBool(c, "Server", "Chroot", false)) {
		if (chroot(webRoot)) {
			perror("chroot");
			exit(EXIT_FAILURE);
		}
		if (chdir("/")) {
			perror("chdir");
			exit(EXIT_FAILURE);
		}
		string_format(webRoot, "/");
	}

	server = server_init((*getHandler)(), ip, port, webRoot);
	while (true) {
		server_freeDeadClients(server);
		server_handleRequests(server);
	}
	server_free(server);


	log_free();
	config_free(c);

	return 0;
}
