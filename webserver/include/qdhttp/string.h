#ifndef CSTRING_H
#define CSTRING_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

struct string {
	char* str;
	size_t length;

	// private
	bool _ownsStr; ///< Should \p str be \c free()'d?
	size_t _capacity; ///< The size of the \p str allocation. (0 = don't own the memory)
};

struct string* string_init();
void string_free(struct string* str);
void string_fromCStr(struct string* str, const char* text);
const char * string_toCStr(struct string* str);

//! Minimize allocation
#define string_minimize(str) string_resize(str, str->length)
#define string_reserve(str, size) string_resize(str, max(str->_capacity, size))

void string_resize(struct string* str, size_t size);

void string_format(struct string* str, const char* fmt, ...);
void string_append(struct string* str, const char* text);
void string_append_format(struct string* str, const char* fmt, ...);

#endif
