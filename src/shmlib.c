/*
    Writed by Sun, Diansen.  <2019.12>, email: diansensun@gmail.com

    This file provides utilities for share memory operation based on POSIX Share Memory.
    To untilize without error, you might add "-lrt" option onto the linker.
*/
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include "report.h"
#include "shmlib.h"

/*
    Only call this func one time, otherwise it may cause content confused.
*/
void* SHM_alloc(char* shm_name, size_t len)
{
    int fd = shm_open(shm_name, O_RDWR|O_CREAT,0644);
    if(fd < 0)
    {
        printf("create share memory error.");
        return NULL;
    }

    if(ftruncate(fd,len)!=0)
    {
        sac_warning("truncate share memory error.");
        return NULL;
    }

    void* shm_vir_addr = mmap(NULL,len,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(shm_vir_addr)
        mlock(shm_vir_addr,len);
    return shm_vir_addr;
}

int SHM_free(char* shm_name,void* shm_vir_addr,long len)
{
    if(munmap(shm_vir_addr,len)<0 || munlock(shm_vir_addr,len)<0)
        return -1;

    int fd = shm_open(shm_name, O_RDWR,0644);
    if(fd < 0)
        return -1;
    /* something uncompleted */
    char filelock[50];
    sprintf(filelock,"/dev/%s_lock",shm_name);
    shm_unlink(shm_name);
    unlink(filelock);
    return 0;
}

void* SHM_get(char* shm_name,size_t len)
{
    int fd = shm_open(shm_name,O_RDWR,0644);
    if(fd < 0)
        return NULL;
    void* shm_vir_addr = mmap(NULL,len,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(shm_vir_addr)
        mlock(shm_vir_addr,len);
    return shm_vir_addr;
}



/*************************
*** Multi-process MUTEX **
**************************/
/*
    The param 'lock' must be created in SHARE MEMORY, otherwise it will not work for locking.
*/
int SHM_mutex_init(pthread_mutex_t* lock)
{
    pthread_mutexattr_t ma;
    pthread_mutexattr_init(&ma);
    pthread_mutexattr_setpshared(&ma, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);

    return pthread_mutex_init(lock, &ma);
}

void SHM_mutex_lock(pthread_mutex_t* lock)
{
    if(pthread_mutex_lock(lock) == EOWNERDEAD)
    {
        sac_warning("will consistent mutex, please check if any process has terminated while holding this mutex.");
        pthread_mutex_consistent(lock);
    }
}

void SHM_mutex_unlock(pthread_mutex_t* lock)
{
    pthread_mutex_unlock(lock);
}

///*
//  GLOBAL LOCK provide you a reliable way for multi-process to coordinate their FIRST resourse competition.
//  notice that:
//    (1)This function use a SHARE variable 'GLOBAL_UNI_LOCK', you need to aviod reusing the same name lock in your code.
//    (2)You only use this function once in a short period. I suggest only use it to lock when initializing the SHARED variable.
//*/
//static void SHM_GLOBAL_UNI_LOCK()
//{
//    int n=0;
//    while(shm_open(GLOBAL_UNI_LOCK,O_CREAT|O_EXCL,0644)<0)
//    {
//        printf("try lock %d time.\n",n+1);
//        sleep(1);
//        n++;
//        if(n>=10)
//        {
//            printf("global lock time out!\n");
//            exit(1);
//        }
//    }
//}
//
//static void SHM_GLOBAL_UNI_UNLOCK()
//{
//    if(shm_unlink(GLOBAL_UNI_LOCK)<0)
//    {
//        sac_warning("global unlock error");
//        exit(1);
//    }
//}

int SHM_trylock(char* lockname)
{
    if(shm_open(lockname,O_CREAT|O_EXCL,0644)<0)
        return -1;
    return 1;
}

int SHM_lock(char* lockname)
{
    int n = 0;
    while(shm_open(lockname,O_CREAT|O_EXCL,0644)<0)
    {
        char msg[50];
        sprintf(msg,"trying lock '%s': %d times",lockname,++n);
        sac_info(msg);
        sleep(1);
    }
    return n;
}

int SHM_lock_n_check(char* lockname)
{
    int n = 0;
    while(shm_open(lockname,O_CREAT|O_EXCL,0644)<0)
    {
        char msg[50];
        sprintf(msg,"trying lock '%s': %d times",lockname,++n);
        sac_info(msg);
        sleep(1);
    }

    char lock[50];
    char chk[50];
    sprintf(lock,"/dev/shm/%s",lockname);
    sprintf(chk,"/dev/shm/%s_chk",lockname);
    int l = link(lock,chk);
    return l;
}

int SHM_unlock(char* lockname)
{
    return shm_unlink(lockname);
}
