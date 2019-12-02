#include "string.h"
#include "report.h"
#include "statusDef.h"
#include "unistd.h"
#include <stdlib.h>
FILE* LogFile;
void info(char* str)
{
    printf("process [%d]: %s\n",getpid(),str);
}

int usr_warning(char* str)
{
    printf("process [%d]: %s, errno: %d\n",getpid(),str, errno);
    return errno;
}

void usr_error(char* str)
{
    printf("process [%d]: %s, errno: %d\n",getpid(),str, errno);
    exit(EXIT_FAILURE);
}

int warnning(char* str)
{
    printf("process [%d]: %s\n",getpid(),str);
}

int _Log(char* log, FILE* file)
{
#ifdef LOG_ALLOW
    return fwrite(log,strlen(log),1,file);
#endif // LOG_ALLOW
}
