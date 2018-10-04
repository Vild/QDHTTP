#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <qdhttp/string.h>

struct string* string_init() {
	struct string* str = malloc(sizeof(struct string));
	str->str = NULL;
	str->length = 0;
	str->_ownsStr = false;
	str->_capacity = 0;
	return str;
}
void string_free(struct string* str) {
	free(str->str);
	free(str);
}
void string_fromCStr(struct string* str, const char* text) {
	str->str = strdup(text);
	str->length = strlen(text);
	str->_ownsStr = false;
	str->_capacity = 0;
}
const char * string_toCStr(struct string* str) {
	return str->str;
}

void string_resize(struct string* str, size_t size) {
	if (str->_capacity == size)
		return;
	str->str = realloc(str->str, str->length);
	str->length = size;
}

void string_format(struct string* str, const char* fmt, ...) {
	va_list va;
	va_start(va, fmt);
	vsnprintf(str->str, str->_capacity, fmt, va);
	va_end(va);
}

void string_append(struct string* str, const char* text) {
	strncat(str->str, text, str->_capacity);
}

void string_append_format(struct string* str, const char* fmt, ...) {
	va_list va;
	va_start(va, fmt);
	vsnprintf(str->str + str->length, str->_capacity - str->length, fmt, va);
	va_end(va);
}
