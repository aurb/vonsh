#include "error_handling.h"
#include "types.h"
#include <stdio.h>
#include <stdarg.h>


static char g_error_message_buffer[1024];
static const char* g_first_error = NULL;

void set_error(const char *fmt, ...) {
    if (g_first_error != NULL) return; // An error has already been recorded.

    va_list args;
    va_start(args, fmt);
    vsnprintf(g_error_message_buffer, sizeof(g_error_message_buffer), fmt, args);
    va_end(args);

    g_first_error = g_error_message_buffer;
    g_game.state = NotInitialized; // This state will cause the main loop to terminate
}

const char* get_first_error(void) {
    return g_first_error;
}
