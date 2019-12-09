#include "string.h"
#include "report.h"
#include "statusDef.h"
#include "unistd.h"
#include <stdlib.h>
FILE* LogFile;
void sac_info(char* format, ...)
{
    printf("process [%d]: ",getpid());
    printf(format);
}

int sac_warning(char* format, ...)
{
    printf("process [%d], errno: %d:",getpid(), errno);
    printf(format);
    return errno;
}

void sac_error_exit(char* format, ...)
{
    printf("process [%d], errno: %d:",getpid(), errno);
    printf(format);
    exit(EXIT_FAILURE);
}


void sac_exit(int flag)
{
    exit(flag);
}


int sac_log(char* log, FILE* file)
{
#ifdef LOG_ALLOW
    return fwrite(log,strlen(log),1,file);
#endif // LOG_ALLOW
}
