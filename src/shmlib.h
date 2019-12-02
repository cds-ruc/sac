#ifndef _SHMLIB_H
#define _SHMLIB_H 1

extern void* SHM_alloc(char* shm_name,size_t len);
extern int SHM_free(char* shm_name,void* shm_vir_addr,long len);
extern void* SHM_get(char* shm_name,size_t len);

extern int SHM_lock_n_check(char* lockname);
extern int SHM_unlock(char* shm_name);

extern int SHM_mutex_init(pthread_mutex_t* lock);
extern void SHM_mutex_lock(pthread_mutex_t* lock);
extern void SHM_mutex_unlock(pthread_mutex_t* lock);

#endif
