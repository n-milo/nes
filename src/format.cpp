#include "format.h"

#include <cstdarg>

std::string string_printf(const char *fmt, ...) {
    va_list ap1, ap2;

    va_start(ap1, fmt);
    int n = vsnprintf(nullptr, 0, fmt, ap1);

                                          // because we can't stop sprintf from writing nul-terminators, we have to:
    std::string str(n+1, '\0');           // allocate a string with n+1 length,
    va_copy(ap2, ap1);
    vsnprintf(str.data(), n+1, fmt, ap2); // then vsnprintf will overwrite that n+1'th character with '\0',
    va_end(ap2);
    va_end(ap1);
    str.erase(str.end()-1);               // finally, erase the last character because we don't want any extra NULs
                                          // (std::string already gives us one at str.end())

    ASSERT(std::all_of(str.begin(), str.end(), [](auto c) { return c != '\0'; }),
           "vsprintf should have overwritten all the NUL characters");
    ASSERT(str.length() == n, "string length should be n");

    return str;
}