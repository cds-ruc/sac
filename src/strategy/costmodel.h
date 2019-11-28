#ifndef COSTMODEL_H
#define COSTMODEL_H
#include <unistd.h>
#include <stdlib.h>

#include "../global.h"
#include "../cache.h"
#include "../timerUtils.h"
#include "../report.h"
extern blkcnt_t TS_WindowSize;
extern blkcnt_t TS_StartSize;
extern int CM_Init();
extern int CM_Reg_EvictBlk(SSDBufTag blktag, unsigned flag, microsecond_t usetime);
extern int CM_TryCallBack(SSDBufTag blktag);

extern int CM_T_rand_Reg(microsecond_t usetime);
extern int CM_T_hitmiss_Reg(microsecond_t usetime);
/** Calling for Strategy **/
extern int CM_CHOOSE();
extern void ReportCM();
extern void CM_Report_PCB();
#endif // COSTMODEL_H


