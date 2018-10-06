#include <qdhttp/socket.h>

#include <qdhttp/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct Client* client_init(int fd, time_t currentTime, struct sockaddr_in addr) {
	struct Client* client = malloc(sizeof(struct Client));
	client->fd = fd;
	client->dead = false;
	client->request = string_init(0x400);
	client->connectAt = currentTime;
	client->addr = addr;
	return client;
}

void client_free(struct Client* client) {
	printf("Freeing client %s:%d\n", inet_ntoa(client->addr.sin_addr), (int)ntohs(client->addr.sin_port));
	close(client->fd);
	string_free(client->request);
	free(client);
}

void client_update(struct Client* client, time_t currentTime) {
	client->dead |= (currentTime - client->connectAt) >= dropClientAfter;
	if (client->dead)
		printf("Client %s:%d is dead\n", inet_ntoa(client->addr.sin_addr), (int)ntohs(client->addr.sin_port));

	if (!client->dead && string_getSize(client->request)) {
		printf("Client %s:%d got a request:\n%s\n", inet_ntoa(client->addr.sin_addr), (int)ntohs(client->addr.sin_port), client->request);
		string response = string_init(0x1000);
		string_append(response, "HTTP/1.0 200 OK\r\n");


		struct tm* t = gmtime(&currentTime);

		static const char wday_name[][4] = {
			"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
		};
		static const char mon_name[][4] = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
		};
		//Fri, 05 Oct 2018 23:14:49 GMT
		string_append_format(response, "Date: %.3s, %.2d %.3s %.4d %.2d:%.2d:%.2d GMT\r\n",
												 wday_name[t->tm_wday],
												 t->tm_mday,
												 mon_name[t->tm_mon],
												 1900 + t->tm_year,
												 t->tm_hour,
												 t->tm_min,
												 t->tm_sec);
		string_append(response, "Server: QDHTTP Version (idk)\r\n");
		string_append(response, "Content-type: text/html\r\n");
		string_append(response, "Connection: close\r\n");
		string_append(response, "\r\n");

		//TODO: parse request
		string requestHeader = string_initFromCStr("GET / HTTP/1.0");
		string useragent = string_initFromCStr("w3m/0.5.3+git20180125");
		string requestURL = string_initFromCStr("index.html");
		string data = NULL;

		{
			string path = string_init(512);
			string_append(path, "../www/");
			string_append(path, requestURL);
			//TODO: validate path

			FILE* fp = fopen(path, "rb");
			string_free(path);

			fseek(fp, 0, SEEK_END);
			size_t len = (size_t)ftell(fp);
			rewind(fp);

			data = string_init(len);
			string_setSize(data, len);
			data[len] = '\0';
			fread(data, (size_t)len, 1, fp);
			fclose(fp);
		}

		string_append(response, data);

		log_access("0.0.0.0", currentTime, requestHeader, 200, string_getSize(data), requestURL, useragent);

		string_free(data);
		string_free(requestURL);
		string_free(useragent);
		string_free(requestHeader);

		printf("response: '%s'", response);
		write(client->fd, response, string_getSize(response));

		client->dead = true;
	}
}

struct Server* server_init(string ip, uint16_t port, size_t clientCapacity) {
	struct Server* server = malloc(sizeof(struct Server));

	server->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	int option = 1;
	setsockopt(server->fd, SOL_SOCKET , SO_REUSEADDR, (char *)&option, sizeof(option));

	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	inet_aton(ip, &address.sin_addr);

	if (bind(server->fd, (struct sockaddr *)&address , sizeof(address))) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(server->fd, (int)clientCapacity)) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	server->clients = malloc(sizeof(struct Client*) * clientCapacity);
	server->clientsOther = malloc(sizeof(struct Client*) * clientCapacity);
	server->clientCount = 0;
	server->clientCapacity = clientCapacity;

	return server;
}

void server_free(struct Server* server) {
	for (size_t i = 0; i < server->clientCount; i++)
		client_free(server->clients[i]);
	free(server->clients);
	free(server->clientsOther);
	close(server->fd);
	free(server);
}

void server_freeDeadClients(struct Server* server) {
	size_t removed = 0;
	struct Client** list = server->clientsOther;
	for (size_t i = 0; i < server->clientCount; i++) {
		struct Client* c = server->clients[i];

		if (c->dead) {
			client_free(c);
			removed++;
		} else {
			list[i - removed] = c;
		}
	}
	server->clientCount -= removed;
	server->clientsOther = server->clients;
	server->clients = list;
}

void server_aquireNewClients(struct Server* server) {
	int highestFD = server->fd;

	FD_ZERO(&server->readfds);
	FD_SET(server->fd, &server->readfds);

	for (size_t i = 0; i < server->clientCount; i++) {
		struct Client* client = server->clients[i];

		FD_SET(client->fd, &server->readfds);
		highestFD = max(highestFD, client->fd);
	}

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 250 * 1000;

	//printf("select\n");
	int r = select(highestFD + 1, &server->readfds, NULL, NULL, &tv);
	if (r <= 0)
		return;

	//printf("server isset\n");
	if (FD_ISSET(server->fd, &server->readfds) && server->clientCount < server->clientCapacity) {
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(addr);
		int clientFD = accept(server->fd, (struct sockaddr*)&addr, &addrlen);

		printf("Client from %s:%d connect!\n", inet_ntoa(addr.sin_addr), (int)ntohs(addr.sin_port));
		if (clientFD > 0) {
			struct Client* client = client_init(clientFD, time(NULL), addr);
			server->clients[server->clientCount++] = client;
		}
	}

}

void server_handleRequests(struct Server* server) {
	time_t currentTime = time(NULL);

	for (size_t i = 0; i < server->clientCount; i++) {
		struct Client* client = server->clients[i];
		if (FD_ISSET(client->fd, &server->readfds)) {
			ssize_t amount = read(client->fd, client->request, string_getSpaceLeft(client->request));
			if (amount <= 0)
				client->dead = true;
			printf("Read %ld bytes for client from %s:%d\n", amount, inet_ntoa(client->addr.sin_addr), (int)ntohs(client->addr.sin_port));
			string_setSize(client->request, (size_t)amount);
		}
		client_update(client, currentTime);
	}
}
