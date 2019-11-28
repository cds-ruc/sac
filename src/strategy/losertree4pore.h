/*
    Author: Sun Diansen.    dssun@ruc.edu.cn
    Date: 2017.07.28

*/
#ifndef _LOSERTREE4OPORE_H_
#define _LOSERTREE4OPORE_H_
#include "pore.h"

typedef struct NonLeaf
{
    long value;
    int pathId;
    long despId;
}NonLeaf;

typedef struct LoserTreeInfo
{
    NonLeaf* non_leafs;
    int nonleaf_count;
    int winnerPathId;
}LoserTreeInfo;


extern long LoserTree_Create(int npath, Dscptr** openBlkDesps, void** passport,int* winnerPathId, long* winnerDespId);
extern long LoserTree_GetWinner(void* passport, Dscptr* candidateDesp, int* winnerPathId, long* winnerDespId);
extern int LoserTree_Destory(void* passport);

#endif // _LOSERTREE4OPORE_
