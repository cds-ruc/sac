#ifndef SSDBUFTABLE_H
#define SSDBUFTABLE_H

#include "simulator_v2.h"
typedef struct SSDHashBucket
{
        DespTag	hash_key;
        long    despId;
        struct SSDHashBucket *next_item;
} SSDHashBucket;

extern void initSSDTable(size_t size);
extern unsigned long ssdtableHashcode(DespTag tag);
extern long ssdtableLookup(DespTag tag, unsigned long hash_code);
extern long ssdtableInsert(DespTag tag, unsigned long hash_code, long despId);
extern long ssdtableDelete(DespTag tag, unsigned long hash_code);
extern long ssdtableUpdate(DespTag tag, unsigned long hash_code, long despId);
#endif   /* SSDBUFTABLE_H */
