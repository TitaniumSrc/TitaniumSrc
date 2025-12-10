#if !defined(TISRC_RCMGRALLOC_H) && !defined(TISRC_REUSABLE)
#define TISRC_RCMGRALLOC_H

#include <stddef.h>
#include <stdlib.h>

void* rcmgr_malloc(size_t);
void* rcmgr_calloc(size_t, size_t);
void* rcmgr_realloc(void*, size_t);

#ifndef TISRC_RCMGRALLOC_NOREDEF
    #define malloc rcmgr_malloc
    #define calloc rcmgr_calloc
    #define realloc rcmgr_realloc
#endif

#endif
