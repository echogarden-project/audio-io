#pragma once

#include <cstdarg>
#include <stdio.h>

#ifdef TRACE
auto traceEnabled = true;
#else
auto traceEnabled = false;
#endif

void trace(const char* format, ...) {
	if (traceEnabled) {
		// Initialize the va_list
		va_list args;
		va_start(args, format);

		// Call vprintf to handle the variable arguments
		vprintf(format, args);

		// Clean up the va_list
		va_end(args);
	}
}
