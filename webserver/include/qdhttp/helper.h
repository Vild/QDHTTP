#ifndef HELPER_H_
#define HELPER_H_

#include <string.h>

#ifndef inline
#define inline __inline__
#endif

#ifndef max
#define max(a, b) (a > b ? a : b)
#endif

#ifndef min
#define min(a, b) (a < b ? a : b)
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define ZERO_STRUCT(obj) memset(&obj, 0, sizeof(obj))

#endif
