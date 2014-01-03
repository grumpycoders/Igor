#pragma once

#include <varargs.h>
#include <functional>

bool igor_export_to_text(std::function<bool(const char * fmt, va_list ap)> output, IgorSession * session);
bool igor_export_to_file(const char * exportPath, IgorSession * session);
