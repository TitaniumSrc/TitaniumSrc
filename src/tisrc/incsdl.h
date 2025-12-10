#ifndef TISRC_INCSDL_H
#define TISRC_INCSDL_H

#include "platform.h"

#if PLATFORM == PLAT_ANDROID || PLATFORM == PLAT_GDK
    #include <SDL.h>
#elif defined(TISRC_USESDL1)
    #include <SDL/SDL.h>
#else
    #include <SDL2/SDL.h>
#endif

#endif
