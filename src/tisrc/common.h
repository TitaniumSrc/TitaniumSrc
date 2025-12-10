#ifndef TISRC_COMMON_H
#define TISRC_COMMON_H

#include "versioning.h"
#include "version.h"
#include "string.h"
#include "debug.h"

#include "common/config.h"

extern unsigned quitreq;

extern struct gameinfo {
    char* dir;
    char* name;
    char* author;
    char* desc;
    #ifndef TISRC_MODULE_SERVER
    char* icon;
    #endif
    struct version version;
    struct version minver;
    #ifndef TISRC_MODULE_SERVER
    char* userdir;
    #endif
} gameinfo;

enum dir {
    DIR_MAIN,        // instance dir
    DIR_INTERNAL,    // internal data dir; 'internal' in the main dir
    DIR_INTERNALRC,  // internal:/ resources; 'resources' in the internal data dir
    DIR_GAMES,       // game dirs and game:/ resources; 'games' in the main dir
    DIR_MODS,        // mods and mod:/ resources; 'mods' in the main dir
    DIR_GAME,        // game dir
    #ifndef TISRC_MODULE_SERVER
    DIR_USER,        // user data dir; NULL if there is no suitable user file storage
    DIR_USERRC,      // user:/ resources; 'resources' in the user data dir; NULL if the user data dir is NULL
    DIR_USERMODS,    // user mods and mod:/ resources; 'mods' in the user data dir; NULL if the user data dir is NULL
    DIR_SCREENSHOTS, // 'screenshots' in the user data dir; NULL if the user data dir is NULL or there is no writeable
                     // filesystem suitable for large files
    DIR_SAVES,       // save location; platform-specific dir or 'saves' dir in the user data dir; NULL if there is no
                     // writable filesystem sutiable for saves
    #endif
    DIR_CONFIGS,     // 'configs' in the user data dir, or in 'data/<userdir name>/' in the main dir if the user data
                     // dir is NULL
    DIR_DATABASES,   // 'databases' in the user data dir, or in 'data/<userdir name>/' in the main dir if the user data
                     // dir is NULL
    #ifndef TISRC_MODULE_SERVER
    DIR_SVDL,        // typically 'server' in 'downloads' in the user data dir; NULL if the user data dir is NULL
    DIR_PLDL,        // typically 'player' in 'downloads' in the user data dir; NULL if the user data dir is NULL
    #endif
    DIR__COUNT
};
extern char* dirs[DIR__COUNT];
extern char* dirdesc[DIR__COUNT];

extern struct cfg config;

#define TISRC_COMMON_PBPREPROCVARS_BASE \
    {.name = "tisrc:build", .namecrc = 0xB52F9B10, .data.type = PB_PREPROC_TYPE_U32, .data.u32 = TISRC_BUILD},\
    {.name = "tisrc:module:engine", .namecrc = 0x18735912, .data.type = PB_PREPROC_TYPE_U8, .data.u8 = 0},\
    {.name = "tisrc:module:server", .namecrc = 0xAAB69669, .data.type = PB_PREPROC_TYPE_U8, .data.u8 = 1},\
    {.name = "tisrc:module:editor", .namecrc = 0x3C2AB225, .data.type = PB_PREPROC_TYPE_U8, .data.u8 = 2}
#define TISRC_COMMON_PBPREPROCVARCT_BASE (4)

#if DEBUG(0)
    #define TISRC_COMMON_PBPREPROCVARS_DEBUG \
        {.name = "tisrc:debug", .namecrc = 0x642A2E6C, .data.type = PB_PREPROC_TYPE_U8, .data.u8 = TISRC_DBGLVL},
    #define TISRC_COMMON_PBPREPROCVARCT_DEBUG (1)
#else
    #define TISRC_COMMON_PBPREPROCVARS_DEBUG
    #define TISRC_COMMON_PBPREPROCVARCT_DEBUG (0)
#endif

#define TISRC_COMMON_PBPREPROCVARS \
    TISRC_COMMON_PBPREPROCVARS_DEBUG\
    TISRC_COMMON_PBPREPROCVARS_BASE
#define TISRC_COMMON_PBPREPROCVARCT (TISRC_COMMON_PBPREPROCVARCT_DEBUG + TISRC_COMMON_PBPREPROCVARCT_BASE)

#endif
