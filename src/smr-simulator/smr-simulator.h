//#ifndef SMR_SSD_CACHE_SMR_SIMULATOR_H
//#define SMR_SSD_CACHE_SMR_SIMULATOR_H
//
//#include "global.h"
//#include "statusDef.h"
//
//#define DEBUG 0
///* ---------------------------smr simulator---------------------------- */
//#include <pthread.h>
//
//typedef struct
//{
//    off_t offset;
//} DespTag;
//
//typedef struct
//{
//    DespTag tag;
//    long    despId;
//    long    pre_useId;
//    long    next_useId;
//    long    next_freeId;
//} FIFODesc;
//
//
//
//typedef struct
//{
//	unsigned long	n_used;
//	long		    first_useId;		// Head of list of used
//	long		    last_useId;		// Tail of list of used
//	long            first_freeId;
//	long            last_freeId;
//} FIFOCtrl;
//
//extern int  fd_fifo_part;
//extern int  fd_smr_part;
//
//extern void initFIFOCache();
//extern int smrread(char* buffer, size_t size, off_t offset);
//extern int smrwrite(char* buffer, size_t size, off_t offset);
//extern void PrintSimulatorStatistic();
//
//#endif
