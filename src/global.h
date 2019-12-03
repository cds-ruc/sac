#ifndef _GLOBAL_H
#define _GLOBAL_H 1

#include <sys/types.h>
#include "statusDef.h"

#define SSD_BUF_VALID 0x01
#define SSD_BUF_DIRTY 0x02

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

typedef enum enum_workload_mode
{
    IOMODE_R, // 0
    IOMODE_W, // 1
    IOMODE_RW // 2 *default
}enum_workload_mode;

typedef enum
{
//   LRU,
    PAUL,
    MOST,
    MOST_CDC,
    LRU_private,
    LRU_CDC
}SSDEvictionStrategy;


/** This user basic info */
int TraceID = -1;
FILE* TraceFile = NULL;
off_t StartLBA = 0;
enum enum_workload_load Workload_Mode = IOMODE_RW; // *Default
SSDEvictionStrategy EvictStrategy = PAUL; // *Default
long Cycle_Length;

int NO_REAL_DISK_IO = 0;
int NO_CACHE = 0;
int EMULATION = 0;

/** ENV**/
struct RuntimeSTAT* STT;
blksize_t BLKSZ = 4096;

// Cache Layer
blksize_t NBLOCK_SSD_CACHE = 8000000; // 32GB
blksize_t NTABLE_SSD_CACHE; // equal with NBLOCK_SSD_CACHE

// SMR layer
blksize_t NBLOCK_SMR_PB = 30 * 5000;
blkcnt_t  NZONES = 400000;/* size = 8TB */ //194180;    // NZONES * ZONESZ =
blksize_t ZONESZ = 5000 * 4096;//20MB    // Unit: Byte.

// Device Files
char* simu_smr_fifo_device = NULL;// "/mnt/smr/pb";
char* simu_smr_smr_dev_path = NULL;//"/mnt/smr/smr";
char* smr_dev_path = NULL;//"/mnt/smr/smr-rawdisk"; // /dev/sdc";
char* cache_dev_path = NULL;//"/mnt/ssd/ssd";//"/mnt/ramdisk/ramdisk";//"/dev/memdiska";// "/mnt/ssd/ssd";

int cache_fd = -1;
int smr_fd = -1;

/* Logs */
char* PATH_LOG = PROJ_ROOT + "/logs/";
char log_emu_path[] = "../../logs/log_emu";
FILE* log_emu;

/** Shared memory variable names **/
// Note: They are legacy from multi-user version, and are not used in this code. 
char* SHM_SSDBUF_STRATEGY_CTRL = "SHM_SSDBUF_STRATEGY_CTRL";
char* SHM_SSDBUF_STRATEGY_DESP = "SHM_SSDBUF_STRATEGY_DESP";

char* SHM_SSDBUF_DESP_CTRL = "SHM_SSDBUF_DESP_CTRL";
char* SHM_SSDBUF_DESPS = "SHM_SSDBUF_DESPS";

char* SHM_SSDBUF_HASHTABLE_CTRL = "SHM_SSDBUF_HASHTABLE_CTRL";
char* SHM_SSDBUF_HASHTABLE = "SHM_SSDBUF_HASHTABLE";
char* SHM_SSDBUF_HASHDESPS =  "SHM_SSDBUF_HASHDESPS";
char* SHM_PROCESS_REQ_LOCK = "SHM_PROCESS_REQ_LOCK";
#endif







