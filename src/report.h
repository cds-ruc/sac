#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
extern void sac_info(char* format, ...);
extern int sac_warning(char* format, ...);
extern void sac_error_exit(char* format, ...);
extern int sac_log(char* log, FILE* file);
extern void sac_exit(int flag);
