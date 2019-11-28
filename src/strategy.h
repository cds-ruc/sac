#ifndef _STRAtEGY_H_
#define _STRAtEGY_H_

#include <lru.h>
#include <lru_private.h>
#include <lru_batch.h>
struct Ref_lru_global
{
    blksize_t whole_cache_size; // the blocks number of the whole lru cache which shared by all users.
};

struct Ref_lru_private
{
    //blksize_t maxssd;   // this user private lru cache blocks count.
};

union StratetyUnion
{
    struct Ref_lru_global ref_lru_global;
    struct Ref_lru_private ref_lru_private;
};


#endif // _STRAtEGY_H_
