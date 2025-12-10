#ifndef TISRC_DEBUG_H
#define TISRC_DEBUG_H

// TISRC_DBGLVL:
// undef: off
// 0: silent
// 1: basic
// 2: advanced
// 3: detailed
#ifndef TISRC_DBGLVL
    #define DEBUG(x) 0
#else
    #define DEBUG(x) (TISRC_DBGLVL >= (x))
#endif

#endif
