#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

// Function to set an error message, accessible from multiple source files
void set_error(const char *fmt, ...);

// Function to get the first error message
const char* get_first_error(void);

#endif // ERROR_HANDLING_H 