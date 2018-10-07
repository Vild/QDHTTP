#include <qdhttp/log.h>

#include <stdio.h>
#include <syslog.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

static FILE* accessFp = NULL;
static FILE* errorFp = NULL;

#define SECSPERHOUR (60 * SECSPERMIN)
#define SECSPERMIN (60)

void log_init(string* filepath, bool isDaemon) {
	openlog("QDHTTP", LOG_NDELAY, isDaemon ? LOG_DAEMON : LOG_USER);
	if (*filepath) {
		string_reserve(filepath, string_getSize(*filepath) + 4);
		string_append(*filepath, ".log");
		accessFp = fopen(*filepath, "a+");

		string_setSize(*filepath, string_getSize(*filepath) - 3);
		string_append(*filepath, "err");
		errorFp = fopen(*filepath, "a+");
	}
}

void log_access(string host, time_t date, string request, uint16_t status, size_t bytes, string url, string useragent) {
	static const char mon_name[][4] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	struct tm* t = localtime(&date);
	string output = string_init(512);
	string_format(output,	"%s - - [%.2d/%.3s/%.4d:%.2d:%.2d:%.2d %+03ld%.2ld] \"%s\" %d %zu \"%s\" \"%s\"",
								host, t->tm_mday, mon_name[t->tm_mon], 1900 + t->tm_year, t->tm_hour,
								t->tm_min, t->tm_sec, t->tm_gmtoff / SECSPERHOUR,
								labs(t->tm_gmtoff / SECSPERMIN) % 60L, request, status, bytes,
								url, useragent);

	syslog(LOG_INFO, "%s", output);
	if (accessFp)
		fprintf(accessFp, "%s\n", output);
	string_free(output);
}

void log_error(time_t date, string type, string client, string message) {
	static const char wday_name[][4] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static const char mon_name[][4] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	struct tm* t = localtime(&date);
	string output = string_init(512);

	string_format(output,	"[%.3s %.3s %.2d %.2d:%.2d:%.2d %.4d] [%s] [%s] %s",
								wday_name[t->tm_wday], mon_name[t->tm_mon], t->tm_mday,
								t->tm_hour, t->tm_min, t->tm_sec, 1900 + t->tm_year,
								type, client, message);

	syslog(LOG_ERR, "%s", output);
	if (errorFp)
		fprintf(errorFp, "%s\n", output);
	string_free(output);
}

void log_free() {
	closelog();
	if (accessFp) {
		fclose(accessFp);
		fclose(errorFp);
	}
}
