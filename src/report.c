#include "string.h"
#include "report.h"
#include "statusDef.h"
#include "unistd.h"
#include <stdlib.h>
FILE* LogFile;
void paul_info(char* format, ...)
{
    printf("process [%d]: ",getpid());
    printf(format);
}

int paul_warning(char* format, ...)
{
    printf("process [%d], errno: %d:",getpid(), errno);
    printf(format);
    return errno;
}

void paul_error_exit(char* format, ...)
{
    printf("process [%d], errno: %d:",getpid(), errno);
    printf(format);
    exit(EXIT_FAILURE);
}


void paul_exit(int flag)
{
    exit(flag);
}


int paul_log(char* log, FILE* file)
{
#ifdef LOG_ALLOW
    return fwrite(log,strlen(log),1,file);
#endif // LOG_ALLOW
}
