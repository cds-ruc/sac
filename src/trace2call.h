#define DEBUG 0
/* ---------------------------trace 2 call---------------------------- */
#ifndef _TRACETOCALL_H
#define _TRACETOCALL_H 1

#define ACT_READ '0'
#define ACT_WRITE '1'

//#define _CG_LIMIT 1

#endif // TRACETOCALL

extern FILE *log_lat, *log_lat_pb;

extern void trace_to_iocall(char *trace_file_path, int isWriteOnly, off_t startLBA);
extern int BandOrBlock;
