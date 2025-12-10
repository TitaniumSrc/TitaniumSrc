#if defined(TISRC_RCMGRALLOC_H) && !defined(TISRC_REUSABLE)
    #undef malloc
    #undef calloc
    #undef realloc
    #undef TISRC_RCMGRALLOC_H
#endif
