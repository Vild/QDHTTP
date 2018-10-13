#ifndef LOG_H_
#define LOG_H_

#include <qdhttp/string.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>

/// If filepath is NULL, syslog will be used.
void log_init(string filepath, bool isDaemon);
void log_access(const string host, time_t date, const string request, uint16_t status, size_t bytes, const string url, const string useragent);
void log_error(time_t date, const string type, const string client, const string message);
void log_free(void);

#endif
