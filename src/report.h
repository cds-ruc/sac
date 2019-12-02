#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
extern void paul_info(char* format, ...)
extern int paul_warning(char* format, ...)
extern void paul_error_exit(char* format, ...)
extern int paul_log(char* log, FILE* file);
extern void paul_exit(int flag); 
