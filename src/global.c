#include <sys/types.h>
#include <stdio.h> // FILE*
#include <stdlib.h>
#include <string.h>

#include "statusDef.h"
#include "global.h"


/* ENV */


/** This user basic info */
int TraceID = -1;
FILE* TraceFile = NULL;
long int Request_limit = -1;
off_t StartLBA = 0;
int Workload_Mode = IOMODE_RW; // *Default
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
blksize_t NTABLE_SSD_CACHE = 8000000; // equal with NBLOCK_SSD_CACHE

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
char Log_emu_path[] = "./logs/emulator.log";
FILE* Log_emu = NULL;

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
