#ifndef CSTRING_H
#define CSTRING_H

#include <qdhttp/helper.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

typedef char* string;

struct string_header {
	size_t length;
	size_t capacity; ///< The size of the \p str allocation. (0 = don't own the memory)
	char str[0];
};

_Static_assert(sizeof(struct string_header) == 16, "string_header must be 16 bytes in size");

static inline struct string_header* string_getHeader(string str) {
	return ((struct string_header*)str) - 1;
}

string string_init(size_t reservedSize);
string string_initFromCStr(const char* text);
void string_free(string str);

static inline size_t string_getSize(string str) {
	return string_getHeader(str)->length;
}
void string_setSize(string str, size_t len);


static inline size_t string_getCapacity(string str) {
	return string_getHeader(str)->capacity;
}
static inline size_t string_getSpaceLeft(string str) {
	struct string_header* sh = string_getHeader(str);
	return sh->capacity - sh->length;
}

void string_resize(string* str, size_t size);
static inline void string_minimize(string* str) {
	struct string_header* sh = string_getHeader(*str);
	string_resize(str, sh->length);
}
static inline void string_reserve(string* str, size_t size) {
	struct string_header* sh = string_getHeader(*str);
	string_resize(str, max(sh->capacity, size));
}

void string_format(string str, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
void string_append(string str, const char* text);
void string_append_format(string str, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

#endif
