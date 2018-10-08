#include <qdhttp/client.h>
#include <qdhttp/log.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#define dropClientAfter 2 /* Seconds */

struct Client* client_init(int fd, time_t currentTime, struct sockaddr_in addr, string webRoot) {
	struct Client* client = malloc(sizeof(struct Client));
	client->fd = fd;
	client->dead = false;
	client->request = string_init(0x400);
	client->connectAt = currentTime;
	client->addr = addr;
	client->webRoot = webRoot;
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
		printf("Client %s:%d got a request:\n==Request Start==\n%s\n==Request End==\n", inet_ntoa(client->addr.sin_addr), (int)ntohs(client->addr.sin_port), client->request);
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
		string refererURL = string_initFromCStr("http://nosite.com/");
		string data = NULL;

		{
			string path = string_init(512);
			string_append(path, client->webRoot);
			string_append(path, requestURL);
			//TODO: validate path
			printf("Will open: %s\n", path);

			FILE* fp = fopen(path, "rb");
			string_free(path);

			fseek(fp, 0, SEEK_END);
			size_t len = (size_t)ftell(fp);
			rewind(fp);

			data = string_init(len);
			string_setSize(data, len);
			data[len] = '\0';
			len = (size_t)fread(data, len, 1, fp);
			fclose(fp);
		}

		string_append(response, data);

		log_access("0.0.0.0", currentTime, requestHeader, 200, string_getSize(data), refererURL, useragent);

		string_free(data);
		string_free(refererURL);
		string_free(requestURL);
		string_free(useragent);
		string_free(requestHeader);

		printf("response: \n==Response Start==\n%s\n==Response End==\n", response);

		size_t responseLen = string_getSize(response);
		size_t offset = 0;
		do {
			ssize_t sent = write(client->fd, response + offset, responseLen - offset);
			printf("\tresponseLen: %lu\n", responseLen);
			printf("\toffset: %lu\n", offset);
			printf("\tsent: %ld\n", sent);

			if (sent <= 0)
				break; // Socket is always dead after this loop is done

			offset += (size_t)sent;
		} while(offset < responseLen);

		client->dead = true;
	}
}
