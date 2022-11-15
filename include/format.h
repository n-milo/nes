#pragma once

#include <string>

std::string string_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));