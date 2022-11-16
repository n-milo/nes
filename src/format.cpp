#include "format.h"

#include <cstdarg>

std::string string_printf(const char *fmt, ...) {
    va_list ap1, ap2;

    va_start(ap1, fmt);
    int n = vsnprintf(nullptr, 0, fmt, ap1);

    std::string str(n, '\0');
    va_copy(ap2, ap1);
    vsnprintf(str.data(), n+1, fmt, ap2); // vsnprintf will write the null terminator into str[n]
    va_end(ap2);
    va_end(ap1);

    ASSERT(str[str.length()] == 0, "string should be null terminated");
    ASSERT(std::all_of(str.begin(), str.end(), [](auto c) { return c != '\0'; }),
           "vsnprintf should have overwritten all the NUL characters");

    return str;
}