#ifndef LOG_H_
#define LOG_H_

#include <qdhttp/string.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>

/// If filepath is NULL, syslog will be used.
void log_init(const char* filepath, bool isDaemon);
void log_access(string host, time_t date, string request, uint16_t status, size_t bytes, string url, string useragent);
void log_free(void);

#endif
