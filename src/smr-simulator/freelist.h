
#ifndef _FREE_LIST_H
#define _FREE_LIST_H

struct freeItem
{
    unsigned long id;
    int status;
    struct freeItem * next_free;
    struct freeItem * pre_free;

};

struct _freelist_t
{
    struct freeItem* flist;
    struct freeItem* firstFree;
};

typedef struct _freelist_t freelist_t;

#endif
