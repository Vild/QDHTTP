#include <qdhttp/client.h>
#include <qdhttp/log.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define dropClientAfter 2 /* Seconds */

#define HTTPMethod_data(x)											\
	x(HTTPMETHOD_UNKNOWN, "UNKNOWN")							\
		x(HTTPMETHOD_GET, "GET")										\
		x(HTTPMETHOD_HEAD, "HEAD")

enum HTTPMethod {
#define x(name, str) name,
	HTTPMethod_data(x)
#undef x
};

static char* const HTTPMethod_str[] = {
#define x(name, str) str,
	HTTPMethod_data(x)
#undef x
};
#undef HTTPMethod_data


#define HTTPCode_data(x)																						\
	x(HTTPCODE_OK, 200, "OK")																					\
		x(HTTPCODE_BAD_REQUEST, 400, "Bad Request")											\
		x(HTTPCODE_FORBIDDEN, 403, "Forbidden")													\
		x(HTTPCODE_NOT_FOUND, 404, "Not Found")													\
		x(HTTPCODE_INTERNAL_SERVER_ERROR, 500, "Internal Server Error")	\
		x(HTTPCODE_NOT_IMPLEMENTED, 501, "Not Implemented")

enum HTTPCode {
#define x(name, num, str) name,
	HTTPCode_data(x)
#undef x
};

static const uint16_t HTTPCode_num[] = {
#define x(name, num, str) num,
	HTTPCode_data(x)
#undef x
};

static char* const HTTPCode_str[] = {
#define x(name, num, str) str,
	HTTPCode_data(x)
#undef x
};

struct KeyValue {
	string key;
	string value;
};

struct Request {
	string fullRequestLine;
	enum HTTPMethod method;
	string url;
	struct KeyValue* querys;
	size_t queryCount;

	//string httpVersion;
	struct KeyValue* headers;
	size_t headerCount;

	enum HTTPCode requestFulfillmentStatus;
};

static void _freeRequest(struct Request* request) {
	string_free(request->fullRequestLine);
	string_free(request->url);
	for (size_t i = 0; i < request->queryCount; i++) {
		string_free(request->querys[i].key);
		string_free(request->querys[i].value);
	}
	for (size_t i = 0; i < request->headerCount; i++) {
		string_free(request->headers[i].key);
		string_free(request->headers[i].value);
	}
	free(request->headers);
}

static string _getHeader(struct Request* request, const char* key, string defaultValue) {
	for (size_t i = 0; i < request->headerCount; i++)
		if (!strcmp(request->headers[i].key, key))
			return request->headers[i].value;
	return defaultValue;
}

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


/**
 * Extract the HTTP request
 *
 * \param[out] request
 * \return Is if it a valid request.
 */
static bool _extractRequest(struct Request* request, string requestData) {
	if (!string_getSize(requestData))
		return false;

	// Find a empty line. Everything before this line is the request.
	// Ignore the rest.
	size_t requestLength = 0;
	size_t headerCount = 0;
	{
		char* nextLineBeginning = strstr(requestData, "\r\n");
		if (!nextLineBeginning)
			return false;

		nextLineBeginning += 2;

		// If it is not a Simple-Request, more searching is required.
		if (nextLineBeginning != &requestData[string_getSize(requestData)])
			while (true) {
				char* lineEnding = strstr(nextLineBeginning, "\r\n");
				if (!lineEnding)
					lineEnding = &requestData[string_getSize(requestData) - 1];

				if (lineEnding == nextLineBeginning)
					break;

				//TODO: make better
				// If the lineEnding is at the end of the string, the next iterator of
				// this loop will use invalid memory, and the result will not be found.
				if (lineEnding == &requestData[string_getSize(requestData)])
					return false;

				nextLineBeginning = lineEnding + 2;
				headerCount++;
			}

		requestLength = (size_t)nextLineBeginning - (size_t)requestData;
	}

	// Limit the request length as the whole request is found.
	// The request in it self should now be valid, if the data inside the request
	// is invalid, it should have already returned a HTTP error.
	string_setSize(requestData, requestLength);
	request->requestFulfillmentStatus = HTTPCODE_OK;

	// From here on our the request string will be modified
	// The full data can still be access with the help of string_getSize();
	// But the data will contain '\0' due to the usage of the str* functions.

	char* lineSave = NULL;
	char* curLine = strtok_r(requestData, "\r\n", &lineSave);
	request->fullRequestLine = string_initFromCStr(curLine);

	// Parse the request line
	{
		char* requestSave = NULL;
		char* requestLine = strtok_r(curLine, " ", &requestSave);

		if (!requestLine) {
			request->requestFulfillmentStatus = HTTPCODE_BAD_REQUEST;
			return true;
		}

		// Parse HTTP Method
		{
			if (!strcmp(requestLine, "GET"))
				request->method = HTTPMETHOD_GET;
			else if (!strcmp(requestLine, "HEAD"))
				request->method = HTTPMETHOD_HEAD;
			else {
				request->requestFulfillmentStatus = HTTPCODE_NOT_IMPLEMENTED;
				return true;
			}
		}

		// Parse URL & query
		requestLine = strtok_r(NULL, " ", &requestSave);
		if (!requestLine) {
			request->requestFulfillmentStatus = HTTPCODE_BAD_REQUEST;
			return true;
		}
		{
			request->url = string_initFromCStr(requestLine);
			char* queryPart = strchr(request->url, '?');
			if (queryPart) {
				size_t queryCount = 1;
				for (const char* w = queryPart; *w; w++)
					if (*w == '&')
						queryCount++;

				request->queryCount = queryCount;
				request->querys = malloc(sizeof(struct KeyValue) * queryCount);

				char* querySave = NULL;
				char* q = strtok_r(queryPart + 1 /* The '?' */, "&", &querySave);
				size_t i = 0;
				while (q) {
					struct KeyValue* kv = &request->querys[i++];
					char* middle = strchr(q, '=');
					if (middle) {
						*middle = '\0';
						middle++;
					}

					kv->key = string_initFromCStr(q);
					kv->value = string_initFromCStr(middle);
					q = strtok_r(NULL, "&", &querySave);
				}
				string_setSize(request->url, (size_t)queryPart - (size_t)request->url);
			}
		}

		// Parse HTTP version (This exist only for >= HTTP/1.0
		requestLine = strtok_r(NULL, " ", &requestSave);
		if (requestLine) {
			if (strcmp(requestLine, "HTTP/1.0") && strcmp(requestLine, "HTTP/1.1")) {
				request->requestFulfillmentStatus = HTTPCODE_BAD_REQUEST;
				return true;
			}

			requestLine = strtok_r(NULL, " ", &requestSave);
		}

		// Should be end of line here
		if (requestLine) {
			request->requestFulfillmentStatus = HTTPCODE_BAD_REQUEST;
			return true;
		}
	}

	// Parse headers
	request->headerCount = headerCount;
	printf("Will malloc: %zu * %zu\n", sizeof(struct KeyValue), request->headerCount);
	request->headers = malloc(sizeof(struct KeyValue) * request->headerCount);
	size_t idx = 0;
	while ((curLine = strtok_r(NULL, "\r\n", &lineSave))) {
		if (idx == headerCount) {
			fprintf(stderr, "[ASSERT] IDX COUNTER IS TOO BIG\n");
			exit(0);
		}

		char* middle = strchr(curLine, ':');
		// middle[1] is valid as long as middle isn't null
		// Worst case scenario, ':' is the last char, but there will always be a
		// '\0' after it.
		// Expect "key: v a l u e"
		if (!middle && middle[1] == ' ') {
			request->requestFulfillmentStatus = HTTPCODE_BAD_REQUEST;
			return true;
		}

		*middle = '\0';
		middle += 2; // remove ": "

		request->headers[idx].key = string_initFromCStr(curLine);
		request->headers[idx].value = string_initFromCStr(middle);

		idx++;
	}

	return true;
}

static FILE* _convertURLToFilePathAndOpen(struct Request* request, string webRoot) {
	if (request->requestFulfillmentStatus != HTTPCODE_OK)
		return NULL;

	if (!string_getSize(request->url) || request->url[0] != '/') {
		request->requestFulfillmentStatus = HTTPCODE_BAD_REQUEST;
		return NULL;
	}

	// Note this implementation does not support the url in absoluteURI form
	// As according to the RFC it only used for talking to proxys, and this
	// webserver is a webserver and not a proxy.

	string url = string_initFromCStr(request->url);
	string_reserve(&url, 256);

	if (!strcmp(url, "/"))
		string_format(url, "/index.html");
	else {
		// First remove ./
/*		char* src = url + 1;
			char* dst = url + 1;*/
	}

	string path = string_init(512);
	string_append(path, webRoot);
	string_append(path, (path[string_getSize(path) - 1] == '/') ? url + 1 : url);
	printf("Will open: %s\n", path);
	FILE* fp = fopen(path, "r");
	if (!fp) {
		request->requestFulfillmentStatus = HTTPCODE_FORBIDDEN;
		return NULL;
	}

	//request->requestFulfillmentStatus = HTTPCODE_INTERNAL_SERVER_ERROR;
	return fp;
}

void client_update(struct Client* client, time_t currentTime) {
	client->dead |= (currentTime - client->connectAt) >= dropClientAfter;
	if (client->dead)
		printf("Client %s:%d is dead\n", inet_ntoa(client->addr.sin_addr), (int)ntohs(client->addr.sin_port));

	struct Request request;
	memset(&request, 0, sizeof(struct Request));

	if (!client->dead && _extractRequest(&request, client->request)) {
		printf("Client %s:%d got a request:\n", inet_ntoa(client->addr.sin_addr), (int)ntohs(client->addr.sin_port));

		printf("\tfullRequestLine: %s\n", request.fullRequestLine);
		printf("\tmethod: %s\n", HTTPMethod_str[request.method]);
		printf("\turl: %s\n", request.url);
		printf("\tquerys (%zu)\n", request.queryCount);
		for(size_t i = 0; i < request.queryCount; i++)
			printf("\t\t'%s' = '%s'\n", request.querys[i].key, request.querys[i].value);
		//printf("\thttpVersion: %s\n", request.httpVersion);
		printf("\theaders (%zu)\n", request.headerCount);
		for(size_t i = 0; i < request.headerCount; i++)
			printf("\t\t'%s' = '%s'\n", request.headers[i].key, request.headers[i].value);
		printf("\trequestFulfillmentStatus: %s\n", HTTPCode_str[request.requestFulfillmentStatus]);

		string response = string_init(0x1000);
		string_append_format(response, "HTTP/1.0 %.3d %s\r\n", HTTPCode_num[request.requestFulfillmentStatus], HTTPCode_str[request.requestFulfillmentStatus]);

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
		string useragent = _getHeader(&request, "User-Agent", "");
		string refererURL = _getHeader(&request, "Referer", "");
		FILE* requestedFile = _convertURLToFilePathAndOpen(&request, client->webRoot);

		size_t dataSize = 0;
		if (requestedFile) {
			fseek(requestedFile, 0, SEEK_END);
			size_t len = (size_t)ftell(requestedFile);
			rewind(requestedFile);

			string data = string_init(len);
			string_setSize(data, len);
			data[len] = '\0';
			len = (size_t)fread(data, len, 1, requestedFile);
			fclose(requestedFile);

			dataSize = string_getSize(data);
			string_append(response, data);
			string_free(data);
		} else {
			// This error data is inlined into this file, incase the www folder cannot
			// be access, or anything else due to file IO. This way the error message
			// will always be shown and will always be correct html 4.01.
			static const char* errorData = "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n"
				"<html>\n"
				"	<head>\n"
				"		<title>HTTP Error %d - %s</title>\n"
				"		<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n"
				"		<link rel=\"stylesheet\" href=\"css/style.css\">\n"
				"	</head>\n"
				"	<body>\n"
				"		<div id=\"content\">\n"
				"			<h1>HTTP Error %d - %s</h1>\n"
				"			<p>%s</p>\n"
				"		</div>\n"
				"	</body>\n"
				"</html>\n";

			string_append_format(response, errorData, HTTPCode_num[request.requestFulfillmentStatus], HTTPCode_str[request.requestFulfillmentStatus], HTTPCode_num[request.requestFulfillmentStatus], HTTPCode_str[request.requestFulfillmentStatus], "Error explaination");
			log_error(currentTime, "error", inet_ntoa(client->addr.sin_addr), HTTPCode_str[request.requestFulfillmentStatus]);
		}
		log_access(inet_ntoa(client->addr.sin_addr), currentTime, request.fullRequestLine, HTTPCode_num[request.requestFulfillmentStatus], dataSize, refererURL, useragent);

		printf("response: \n==Response Start==\n%s\n==Response End==\n", response);

		size_t responseLen = string_getSize(response);
		size_t offset = 0;
		do {
			ssize_t sent = write(client->fd, response + offset, responseLen - offset);

			if (sent <= 0)
				break; // Socket is always dead after this loop is done

			offset += (size_t)sent;
		} while(offset < responseLen);

		_freeRequest(&request);
		client->dead = true;
	}
}
