#include "global.h"

/** This user basic info */
pid_t Fork_Pid = 0; /* Default 0. If is a HRC process, this must be large than 0 */
int BatchId;
int BatchSize;
int UserId = 0;
char* TraceFile = NULL;
off_t StartLBA = 0;
enum enum_workload_load Workload_Mode = RW; // *Default
SSDEvictionStrategy EvictStrategy = PAUL; // *Default
long Cycle_Length;
unsigned long Param1;
unsigned long Param2;

/** cache system basic setup **/           /** NEED TO BE '#DEFINE' **/
blksize_t BLKSZ = 4096;

// Cache Layer
blksize_t NBLOCK_SSD_CACHE;
blksize_t NTABLE_SSD_CACHE; // equal with NBLOCK_SSD_CACHE

// SMR layer
blksize_t NBLOCK_SMR_PB;
blkcnt_t  NZONES = 400000;/* size = 8TB */ //194180;    // NZONES * ZONESZ =
blksize_t ZONESZ = 5000 * 4096;//20MB    // Unit: Byte.

// Device Files
char simu_smr_fifo_device[] = "/mnt/smr/pb";
char simu_smr_smr_device[] = "/mnt/smr/smr";
char smr_device[] = "/mnt/smr/smr-rawdisk"; // /dev/sdc";
char ssd_device[] = "/mnt/ssd/ssd";//"/mnt/ramdisk/ramdisk";//"/dev/memdiska";// "/mnt/ssd/ssd";
char ram_device[1024];

int BandOrBlock;

/*Block = 0, Band=1*/
int hdd_fd;
int ssd_fd;
int ram_fd;
struct RuntimeSTAT* STT;

/** Shared memory variable names **/
char* SHM_SSDBUF_STRATEGY_CTRL = "SHM_SSDBUF_STRATEGY_CTRL";
char* SHM_SSDBUF_STRATEGY_DESP = "SHM_SSDBUF_STRATEGY_DESP";

char* SHM_SSDBUF_DESP_CTRL = "SHM_SSDBUF_DESP_CTRL";
char* SHM_SSDBUF_DESPS = "SHM_SSDBUF_DESPS";

char* SHM_SSDBUF_HASHTABLE_CTRL = "SHM_SSDBUF_HASHTABLE_CTRL";
char* SHM_SSDBUF_HASHTABLE = "SHM_SSDBUF_HASHTABLE";
char* SHM_SSDBUF_HASHDESPS =  "SHM_SSDBUF_HASHDESPS";
char* SHM_PROCESS_REQ_LOCK = "SHM_PROCESS_REQ_LOCK";

char* PATH_LOG = "/home/outputs/logs";

/** Var for T-Switcher **/

/** Pipes for HRC processes **/
#ifdef HRC_PROCS_N
int PipeEnds_of_MAIN[HRC_PROCS_N];
int PipeEnd_of_HRC;
#endif


