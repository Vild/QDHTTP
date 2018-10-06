#include <qdhttp/log.h>

#include <stdio.h>
#include <syslog.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

static bool isSysLog;
static FILE* logFp;

#define SECSPERHOUR (60 * SECSPERMIN)
#define SECSPERMIN (60)

void log_init(const char* filepath, bool isDaemon) {
	isSysLog = !filepath;
	if (isSysLog)
		openlog("QDHTTP", LOG_NDELAY, isDaemon ? LOG_DAEMON : LOG_USER);
	else
		logFp = fopen(filepath, "a+");
}

void log_access(string host, time_t date, string request, uint16_t status, size_t bytes, string url, string useragent) {
	static const char mon_name[][4] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	struct tm* t = localtime(&date);
	if (isSysLog)
		syslog(LOG_INFO, "%s - - [%.2d/%.3s/%.4d:%.2d:%.2d:%.2d %+03ld%.2ld] \"%s\" %d %zu \"%s\" \"%s\"",
					 host,
					 t->tm_mday, mon_name[t->tm_mon], 1900 + t->tm_year, t->tm_hour, t->tm_min, t->tm_sec,
					 t->tm_gmtoff / SECSPERHOUR, labs(t->tm_gmtoff / SECSPERMIN) % 60L,
					 request,
					 status,
					 bytes,
					 url,
					 useragent);
	else
		fprintf(logFp, "%s - - [%.2d/%.3s/%.4d:%.2d:%.2d:%.2d %+03ld%.2ld] \"%s\" %d %zu \"%s\" \"%s\"\n",
						host,
						t->tm_mday, mon_name[t->tm_mon], 1900 + t->tm_year, t->tm_hour, t->tm_min, t->tm_sec,
						t->tm_gmtoff / SECSPERHOUR, labs(t->tm_gmtoff / SECSPERMIN) % 60L,
						request,
						status,
						bytes,
						url,
						useragent);
}

void log_free() {
	if (isSysLog)
		closelog();
	else
		fclose(logFp);
}
