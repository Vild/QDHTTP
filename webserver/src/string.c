#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <qdhttp/string.h>

string string_init(size_t reservedSize) {
	struct string_header* sh = malloc(sizeof(struct string_header) + reservedSize + 1);
	sh->length = 0;
	sh->capacity = reservedSize;
	sh->str[0] = sh->str[reservedSize + 1] = '\0';
	return sh->str;
}

string string_initFromCStr(const char* text) {
	size_t len = strlen(text);
	struct string_header* sh = malloc(sizeof(struct string_header) + len + 1);

	sh->length = len;
	sh->capacity = len;
	sh->str[len + 1] = '\0';
	strncpy(sh->str, text, len); // Will place a \0 as the end
	return sh->str;
}

void string_free(string str) {
	if (str)
		free(string_getHeader(str));
}

void string_setSize(string str, size_t len) {
	struct string_header* sh = string_getHeader(str);
	sh->length = len;
	sh->str[len] = '\0';
}

void string_resize(string* str, size_t size) {
	struct string_header* sh = string_getHeader(*str);

	if (sh->capacity == size)
		return;

	sh = realloc(sh, sizeof(struct string_header) + size + 1);
	sh->length = min(sh->length, size);
	sh->capacity = size;
	sh->str[size + 1] = '\0';
	*str = sh->str;
}

void string_format(string str, const char* fmt, ...) {
	struct string_header* sh = string_getHeader(str);

	va_list va;
	va_start(va, fmt);
	sh->length += (size_t)vsnprintf(sh->str, sh->capacity, fmt, va);
	va_end(va);
}

void string_append(string str, const char* text) {
	struct string_header* sh = string_getHeader(str);

	strncat(sh->str, text, sh->capacity);

	sh->length += (size_t)strlen(&sh->str[sh->length]);
}

void string_append_format(string str, const char* fmt, ...) {
	struct string_header* sh = string_getHeader(str);

	va_list va;
	va_start(va, fmt);
	sh->length += (size_t)vsnprintf(sh->str + sh->length, sh->capacity - sh->length, fmt, va);
	va_end(va);
}
