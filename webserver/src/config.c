#include <qdhttp/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: use log error

struct Property {
	string name;
	string value;
};

struct Section {
	string name;
	struct Property* properties;
	size_t count;
	size_t capacity;
};

struct Config {
	struct Section* sections;
	size_t count;
	size_t capacity;
};

static void property_free(struct Property* p) {
	string_free(p->name);
	string_free(p->value);
}

static void section_free(struct Section* s) {
	string_free(s->name);
	for (size_t i = 0; i < s->count; i++)
		property_free(&s->properties[i]);
	free(s->properties);
}

static struct Property* _addProperty(struct Section* s) {
	if (s->count == s->capacity) {
		s->capacity += 4;
		s->properties = realloc(s->properties, sizeof(struct Property) * s->capacity);
	}

	struct Property* p = &s->properties[s->count];
	p->name = p->value = NULL;

	s->count++;
	return p;
}

static struct Section* _addSection(Config* c) {
	if (c->count == c->capacity) {
		c->capacity += 4;
		c->sections = realloc(c->sections, sizeof(struct Section) * c->capacity);
	}

	struct Section* s = &c->sections[c->count];
	s->name = NULL;
	s->properties = NULL;
	s->count = s->capacity = 0;
	c->count++;
	return s;
}

Config* config_init(string filepath) {
	Config* config = malloc(sizeof(Config));
	config->sections = NULL;
	config->count = config->capacity = 0;

	struct Section* curSec = NULL;
	FILE* fp = fopen(filepath, "r");
	if (!fp) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	string buffer = string_init(128);
	size_t lineNumber = 0;
	while (!feof(fp)) {
		lineNumber++;
		fgets(buffer, (int)string_getCapacity(buffer), fp);
		string_setSize(buffer, strlen(buffer));
		if (string_getSize(buffer) < 2 || buffer[0] == ';')
			continue;
		if (buffer[string_getSize(buffer)-1] == '\n')
			string_setSize(buffer, string_getSize(buffer) - 1);

		if (buffer[0] == '[') {
			curSec = _addSection(config);
			char* end = strchr(buffer, ']');
			if (!end) {
				fprintf(stderr, "Line %zu: Expected to find ']'!\n", lineNumber);
				exit(EXIT_FAILURE);
			}

			*end = '\0';
			curSec->name = string_initFromCStr(&buffer[1]);
		} else {
			if (!curSec) {
				fprintf(stderr, "Line %zu: Properties must be within a section!\n", lineNumber);
				exit(EXIT_FAILURE);
			}

			struct Property* p = _addProperty(curSec);
			char* middle = strchr(buffer, '=');
			if (!middle) {
				fprintf(stderr, "Line %zu: Expected to find '='!\n", lineNumber);
				exit(EXIT_FAILURE);
			}

			*middle = '\0';

			p->name = string_initFromCStr(buffer);
			p->value = string_initFromCStr(middle + 1);
		}
	}

	fclose(fp);
	return config;
}

void config_free(Config* config) {
	for(size_t i = 0; i < config->count; i++)
		section_free(&config->sections[i]);
	free(config->sections);
	free(config);
}

string config_getProperty(Config* config, string section, string property, string defaultValue) {
	for (size_t i = 0; i < config->count; i++) {
		struct Section* s = &config->sections[i];
		if (!strcmp(s->name, section))
			for (size_t j = 0; j < s->count; j++) {
				struct Property* p = &s->properties[j];
				if (!strcmp(p->name, property))
					return p->value;
			}
	}

	return defaultValue;
}


ssize_t config_getPropertyInt(Config* config, string section, string property, ssize_t defaultValue) {
	string val = config_getProperty(config, section, property, NULL);

	if (!val)
		return defaultValue;

	return atoll(val);
}

bool config_getPropertyBool(Config* config, string section, string property, bool defaultValue) {
	string val = config_getProperty(config, section, property, NULL);

	if (!val)
		return defaultValue;

	return !strcmp(val, "true") || !!atoll(val);
}
