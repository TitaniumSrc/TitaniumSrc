#ifndef TISRC_UTIL_H
#define TISRC_UTIL_H

#define STR__EVAL(STR__EVAL__x) #STR__EVAL__x
#define STR(STR__x) STR__EVAL(STR__x)

#define TOPTR(TOPTR__x) ((void*)(uintptr_t)(TOPTR__x))

#ifndef TISRC_DBGLVL
    #define DEFAULTCASE(DEFAULTCASE__x) case DEFAULTCASE__x
#else
    #define DEFAULTCASE(DEFAULTCASE__x) default
#endif

#endif
