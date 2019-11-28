#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "losertree4pore.h"

#define OVERFLOW_LEAF_VALUE 0x7FFFFFFFFFFFFFFF // Not a elegant way to define the max leaf value.

static int lg2_above(int V)
{
    V--;
    V |= V >> 1;

    V |= V >> 2;
    V |= V >> 4;
    V |= V >> 8;
    V |= V >> 16;
    V++;

    int count = 0;
    while(V!=1)
    {
        V = V >> 1;
        count++;
    }
    return count;
}

static void adjust(LoserTreeInfo* info, Dscptr* winnerDesp)
{
    NonLeaf winnerLeaf, tmpLeaf;
    if(winnerDesp!=NULL)
    {
        winnerLeaf.value = winnerDesp->stamp;
        winnerLeaf.pathId = info->winnerPathId;
        winnerLeaf.despId = winnerDesp->serial_id;
    }
    else
    {
        winnerLeaf.value = OVERFLOW_LEAF_VALUE;
    }

    int parentId = (info->nonleaf_count + info->winnerPathId)/2;
    while(parentId > 0)
    {
        NonLeaf* parentLeaf = info->non_leafs + parentId;
        if(winnerLeaf.value > parentLeaf->value)
        {
            //winner lose, parent win
            tmpLeaf = *parentLeaf;
            *parentLeaf = winnerLeaf;
            winnerLeaf = tmpLeaf;
        }
        parentId /= 2;  // when winner wins, looking for upper level.
    }
    info->non_leafs[0] = winnerLeaf;
    info->winnerPathId = winnerLeaf.pathId;
}


/** \brief
 *  Create and initialize the loser tree for multi-path.
 * \param npath: the paths count.
 * \param openBlkDesps: the first elements' address array of each path. In this version I use the data type by Dscptr.
 * \param passport, winnerPathId, winnerDespId
 * \return The first winner value from the input array.
 *
 */
long
LoserTree_Create(int npath, Dscptr** openBlkDesps, void** passport,int* winnerPathId, long* winnerDespId)
{
    int nlevels = lg2_above(npath);
    int nodes_count = pow(2,nlevels);
    NonLeaf* non_leafs = (NonLeaf*)malloc(sizeof(NonLeaf)*nodes_count);

    LoserTreeInfo* ltInfo = (LoserTreeInfo*)malloc(sizeof(LoserTreeInfo));
    ltInfo->nonleaf_count = nodes_count;
    ltInfo->non_leafs = non_leafs;
    ltInfo->winnerPathId = 0;

    /** initial the loser tree with the min value of each path **/
    int i;
    for(i = 0; i < nodes_count; i++)
    {
        non_leafs[i].pathId = -1;
        non_leafs[i].value = -1;
    }

    for(i = 0; i < npath; i++)
    {
        ltInfo->winnerPathId = i;
        adjust(ltInfo,*(openBlkDesps + i));
    }

    Dscptr maxdesp;
    maxdesp.stamp = OVERFLOW_LEAF_VALUE;
    maxdesp.serial_id = -1;
    for(i = npath; i < nodes_count; i++)
    {
        ltInfo->winnerPathId = i;
        adjust(ltInfo,&maxdesp);
    }

    *winnerPathId = non_leafs[0].pathId;
    *winnerDespId = non_leafs[0].despId;
    *passport = ltInfo;

    if(non_leafs[0].value < 0 || non_leafs[0].value == OVERFLOW_LEAF_VALUE)
        return -1;
    return non_leafs[0].value;
}

/** \brief
 * Get the next one winner.
 * \param passport: which was given when initialization.
 * \param candidateDesp:
    the candidate block descriptor which was decieded according the pathId which the last winner descriptor belongs to.
    Usually choosing the tail descriptor in the winner path(Zone). If the path(zone) empty, just give the MAXVALUE descriptor.
 * \param winnerPathId: return the winner path ID this round.
 * \param winnerDespId: return the winner desp ID this round.
 * \return
 *  0: no error.
 */
long
LoserTree_GetWinner(void* passport, Dscptr* candidateDesp, int* winnerPathId, long* winnerDespId)
{
    LoserTreeInfo* ltInfo = (LoserTreeInfo*)passport;
    adjust(ltInfo,candidateDesp);

    if(ltInfo->non_leafs[0].value < 0 || ltInfo->non_leafs[0].value == OVERFLOW_LEAF_VALUE)
    {
        int i;
        printf("LoserTree Now Values are the following array:\n");
        for(i=0;i<ltInfo->nonleaf_count;i++){
            printf("%ld,",ltInfo->non_leafs[i].value);
        }
        *winnerPathId = -1;
        *winnerDespId = -1;
        return -1;
    }
    *winnerPathId = ltInfo->non_leafs[0].pathId;
    *winnerDespId = ltInfo->non_leafs[0].despId;
    return ltInfo->non_leafs[0].value;
}

int LoserTree_Destory(void* passport)
{
    if(passport == NULL)
        return 0;
    LoserTreeInfo* info =(LoserTreeInfo*)passport;
    free(info->non_leafs);
    free(info);
    return 0;
}
