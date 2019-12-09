
#include <sys/types.h>
#include <stdio.h> // FILE*
#include "statusDef.h"

#ifndef _GLOBAL_H
#define _GLOBAL_H 1

#define SSD_BUF_VALID 0x01
#define SSD_BUF_DIRTY 0x02

/* ENV */

//extern char* PROJ_ROOT;


struct RuntimeSTAT
{
    /** This user basic info */
    unsigned int batchId;
    unsigned int userId;
    unsigned int traceId;
    int workload_mode;
    unsigned long startLBA;
    unsigned long trace_req_amount;
    /** Runtime strategy refered parameter **/
    //union StratetyUnion strategyRef;

    /** Runtime Statistic **/
    blkcnt_t cacheLimit;
    blkcnt_t cacheUsage;

    blkcnt_t reqcnt_s;
    blkcnt_t reqcnt_r;
    blkcnt_t reqcnt_w;

    blkcnt_t hitnum_s;
    blkcnt_t hitnum_r;
    blkcnt_t hitnum_w;

    blkcnt_t load_ssd_blocks;
    blkcnt_t load_hdd_blocks;
    blkcnt_t flush_hdd_blocks;
    blkcnt_t flush_ssd_blocks;
    blkcnt_t flush_clean_blocks;

    double time_read_ssd;
    double time_read_hdd;
    double time_write_ssd;
    double time_write_hdd;

    blksize_t hashmiss_sum;
    blksize_t hashmiss_read;
    blksize_t hashmiss_write;

    blkcnt_t wt_hit_rd, rd_hit_wt;
    blkcnt_t incache_n_clean, incache_n_dirty;

    /* Emulator infos*/
    double wtrAmp_cur;
    double WA_sum;
    unsigned long n_RMW;
};

#define IOMODE_R 0x01
#define IOMODE_W 0x10
#define IOMODE_RW 0x11

typedef enum
{
//   LRU,
    SAC,
    MOST,
    MOST_CDC,
    LRU_private
}SSDEvictionStrategy;


/** This user basic info */
extern int TraceID;
extern FILE* TraceFile;
extern long int Request_limit;
extern off_t StartLBA;
extern int Workload_Mode;
extern SSDEvictionStrategy EvictStrategy;
extern long Cycle_Length;

extern int NO_REAL_DISK_IO;
extern int NO_CACHE;
extern int EMULATION;

/** ENV**/
extern struct RuntimeSTAT* STT;
extern blksize_t BLKSZ;

// Cache Layer
extern blksize_t NBLOCK_SSD_CACHE;
extern blksize_t NTABLE_SSD_CACHE; // equal with NBLOCK_SSD_CACHE

// SMR layer
extern blksize_t NBLOCK_SMR_PB;
extern blkcnt_t  NZONES;
extern blksize_t ZONESZ;

// Device Files
extern char* simu_smr_fifo_device;
extern char* simu_smr_smr_dev_path;
extern char* smr_dev_path;
extern char* cache_dev_path;

extern int cache_fd;
extern int smr_fd;

/* Logs */
extern char Log_emu_path[];
extern FILE* Log_emu;

/** Shared memory variable names **/
// Note: They are legacy from multi-user version, and are not used in this code.
extern char* SHM_SSDBUF_STRATEGY_CTRL;
extern char* SHM_SSDBUF_STRATEGY_DESP;

extern char* SHM_SSDBUF_DESP_CTRL;
extern char* SHM_SSDBUF_DESPS;

extern char* SHM_SSDBUF_HASHTABLE_CTRL;
extern char* SHM_SSDBUF_HASHTABLE;
extern char* SHM_SSDBUF_HASHDESPS;
extern char* SHM_PROCESS_REQ_LOCK;

#endif //_GLOBAL_H







