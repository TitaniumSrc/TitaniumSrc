#ifndef TISRC_ENGINE_COMMON_H
#define TISRC_ENGINE_COMMON_H

#include <stdbool.h>

#include "../string.h"
#include "../common/config.h"

extern struct engine {
    struct {
        char* game;
        char* mods;
        #ifndef TISRC_MODULE_SERVER
        char* icon;
        #endif
        bool set__setup;
        struct cfg set;
        char* maindir;
        char* gamesdir;
        char* modsdir;
        #ifndef TISRC_MODULE_SERVER
        char* userdir;
        #endif
        char* config;
        #ifndef TISRC_MODULE_SERVER
        bool nouserconfig;
        bool nocontroller;
        #endif
    } opt;
} engine;

void setupBaseDirs(void);
bool setGame(const char*, bool maybepath, struct charbuf* err);

#endif
