#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>
#include <unistd.h>
#include <qdhttp/string.h>

typedef struct Config Config;

Config* config_init(string filepath);
void config_free(Config* config);

string config_getProperty(Config* config, string section, string property, string defaultValue);
ssize_t config_getPropertyInt(Config* config, string section, string property, ssize_t defaultValue);
bool config_getPropertyBool(Config* config, string section, string property, bool defaultValue);
#endif
