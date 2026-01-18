//#define LUA_LIB

#include "lua.h"

#include "../lua/lua.h"
#include "../lua/lualib.h"
#include "../lua/lauxlib.h"

#include "datastream.h"
#include "string.h"
#include "logging.h"
#include "version.h"
#include "platform.h"
#include "time.h"
#include "util.h"
#include "crc.h"

#define SETFIELD_INT(SETFIELD__L, SETFIELD__name, SETFIELD__value) do {\
    lua_pushinteger((SETFIELD__L), (SETFIELD__value));\
    lua_setfield((SETFIELD__L), -2, (SETFIELD__name));\
} while (0)
#define SETFIELD_STR(SETFIELD__L, SETFIELD__name, SETFIELD__value) do {\
    lua_pushstring((SETFIELD__L), (SETFIELD__value));\
    lua_setfield((SETFIELD__L), -2, (SETFIELD__name));\
} while (0)
#define SETFIELD_FUNC(SETFIELD__L, SETFIELD__name, SETFIELD__value) do {\
    lua_pushcfunction((SETFIELD__L), (SETFIELD__value));\
    lua_setfield((SETFIELD__L), -2, (SETFIELD__name));\
} while (0)
#define SETGLOBAL_NIL(SETGLOBAL__L, SETGLOBAL__name) do {\
    lua_pushnil((SETGLOBAL__L));\
    lua_setglobal((SETGLOBAL__L), (SETGLOBAL__name));\
} while (0)

#define CHECKARGS(CHECKARGS__L, CHECKARGS__n1, CHECKARGS__n2) do {\
    if (lua_gettop((CHECKARGS__L)) != (CHECKARGS__n1)) return ERR_WANTARGS((CHECKARGS__L), (CHECKARGS__n2));\
} while (0)

#define ERR_WANTARGS(ERR__L, ERR__n) luaL_error((ERR__L), "expected %d arguments to '%s'", (ERR__n), luaGetFunc((ERR__L)))

static inline void setupLuaState_platforms(lua_State* L);
static inline void setupLuaState_logging(lua_State* L);
static inline void setupLuaState_time(lua_State* L);
static inline void setupLuaState_crc(lua_State* L);
static inline void setupLuaState(lua_State* L) {
    lua_newtable(L);

    lua_newtable(L);
        SETFIELD_INT(L, "build", TISRC_BUILD);
        SETFIELD_INT(L, "platform", PLATFORM);
        SETFIELD_INT(L, "platflags", PLATFLAGS);
        SETFIELD_INT(L, "arch", ARCH);
        SETFIELD_STR(L, "archstr", ARCHSTR);
        SETFIELD_INT(L, "byteorder", BYTEORDER);
    lua_setfield(L, -2, "info");

    setupLuaState_platforms(L);
    setupLuaState_logging(L);
    setupLuaState_time(L);
    setupLuaState_crc(L);

    lua_setglobal(L, "tisrc");
}

lua_State* newLuaState(void) {
    lua_State* L = luaL_newstate();
    if (!L) return NULL;

    luaL_openselectedlibs(L, LUA_GLIBK | LUA_MATHLIBK | LUA_STRLIBK | LUA_TABLIBK | LUA_UTF8LIBK, 0);

    SETGLOBAL_NIL(L, "collectgarbage");
    SETGLOBAL_NIL(L, "dofile");
    SETGLOBAL_NIL(L, "load");
    SETGLOBAL_NIL(L, "loadfile");
    SETGLOBAL_NIL(L, "pcall");
    SETGLOBAL_NIL(L, "print");
    SETGLOBAL_NIL(L, "warn");
    SETGLOBAL_NIL(L, "xpcall");

    setupLuaState(L);

    return L;
}
void delLuaState(lua_State* L) {
    lua_close(L);
}

static inline const char* luaGetFunc(lua_State* L) {
    lua_Debug ar;
    if (lua_getstack(L, 0, &ar)) {
        lua_getinfo(L, "n", &ar);
        return ar.name;
    }
    return NULL;
}

static void luaLogTrace(lua_State* L, enum loglevel lvl, const char* s, ...) {
    const char* fn;
    const char* f;
    unsigned l;

    int depth = 1;
    lua_Debug ar;
    while (1) {
        if (lua_getstack(L, depth, &ar)) {
            lua_getinfo(L, "nSl", &ar);
            if (ar.what[0] != 'C' || ar.what[1]) {
                fn = ar.name;
                f = ar.short_src;
                l = ar.currentline;
                break;
            }
            ++depth;
        } else {
            fn = NULL;
            f = "lua";
            l = 0;
            break;
        }
    }

    va_list v;
    va_start(v, s);
    vplog_raw(lvl, fn, f, l, s, v);
    va_end(v);
}

static const char* runLuaStream_read(lua_State* L, void* data, size_t* size) {
    (void)L;
    return ds_readchunk(data, size);
}
bool runLuaStream(lua_State* L, struct datastream* ds, int nargs, int nresults) {
    struct charbuf cb;
    if (!cb_init(&cb, 128)) return false;
    bool retval;
    if (!cb_add(&cb, '=') || !cb_addstr(&cb, ds->name) || !cb_finalize(&cb)) {
        retval = false;
        goto ret;
    }
    if (lua_load(L, runLuaStream_read, ds, cb.data, NULL) || lua_pcall(L, nargs, nresults, 0)) {
        plog(LL_CRIT, "Lua error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        retval = false;
        goto ret;
    }
    retval = true;
    ret:;
    cb_dump(&cb);
    return retval;
}

// Logging

static int luaPlog(lua_State* L) {
    CHECKARGS(L, 3, 2);
    enum loglevel lvl = luaL_checkinteger(L, 2);
    const char* msg = luaL_checkstring(L, 3);
    luaLogTrace(L, lvl, "%s", msg);
    return 0;
}
static int luaPlogTerry(lua_State* L) {
    (void)L;
    luaLogTrace(L, LL_INFO, "%s", "Made by the will of Terry!");
    return 0;
}
static inline void setupLuaState_logging(lua_State* L) {
    lua_newtable(L);

        lua_newtable(L);
            SETFIELD_FUNC(L, "__call", luaPlog);
        lua_setmetatable(L, -2);

        SETFIELD_FUNC(L, "terry", luaPlogTerry);

        SETFIELD_INT(L, "plain", LL_PLAIN);
        SETFIELD_INT(L, "ms", LL_MS);
        SETFIELD_INT(L, "info", LL_INFO);
        SETFIELD_INT(L, "warn", LL_WARN);
        SETFIELD_INT(L, "error", LL_ERROR);
        SETFIELD_INT(L, "crit", LL_CRIT);

        SETFIELD_INT(L, "func", LF_FUNC);
        SETFIELD_INT(L, "funcln", LF_FUNCLN);
        SETFIELD_INT(L, "msgbox", LF_MSGBOX);
        SETFIELD_INT(L, "debug", LF_DEBUG);

    lua_setfield(L, -2, "log");
}

// Platforms

static inline void setupLuaState_platforms(lua_State* L) {
    lua_newtable(L);
        for (unsigned i = 0; i < PLAT__COUNT; ++i) {
            lua_newtable(L);
                SETFIELD_STR(L, "name", platname[i]);
                SETFIELD_STR(L, "id", platid[i]);
                lua_newtable(L);
                    for (size_t j = 0; platdir[i][j]; ++j) {
                        lua_pushstring(L, platdir[i][j]);
                        lua_rawseti(L, -2, j);
                    }
                lua_setfield(L, -2, "dirs");
            lua_rawseti(L, -2, i);
        }
    lua_setfield(L, -2, "platforms");

    lua_newtable(L);
        for (unsigned i = 0; i < PLAT__COUNT; ++i) {
            SETFIELD_INT(L, platid[i], i);
        }
    lua_setfield(L, -2, "platform");

    lua_newtable(L);
        SETFIELD_INT(L, "unixlike", PLATFLAG_UNIXLIKE);
        SETFIELD_INT(L, "windowslike", PLATFLAG_WINDOWSLIKE);
        SETFIELD_INT(L, "is64bit", PLATFLAG_64BIT);
    lua_setfield(L, -2, "platflag");

    lua_newtable(L);
        SETFIELD_INT(L, "unknown", ARCH_UNKNOWN);
        SETFIELD_INT(L, "amd64", ARCH_AMD64);
        SETFIELD_INT(L, "arm", ARCH_ARM);
        SETFIELD_INT(L, "arm64", ARCH_ARM64);
        SETFIELD_INT(L, "i386", ARCH_I386);
        SETFIELD_INT(L, "mips", ARCH_MIPS);
        SETFIELD_INT(L, "ppc", ARCH_PPC);
        SETFIELD_INT(L, "wasm", ARCH_WASM);
    lua_setfield(L, -2, "arch");

    lua_newtable(L);
        SETFIELD_INT(L, "little", BO_LE);
        SETFIELD_INT(L, "big", BO_BE);
    lua_setfield(L, -2, "byteorder");
}

// Time

static int luaDelay(lua_State* L) {
    CHECKARGS(L, 1, 1);
    microwait(luaL_checkinteger(L, 1));
    return 0;
}
static int luaTime(lua_State* L) {
    CHECKARGS(L, 0, 0);
    lua_pushinteger(L, altutime());
    return 1;
}
static inline void setupLuaState_time(lua_State* L) {
    SETFIELD_FUNC(L, "delay", luaDelay);
    SETFIELD_FUNC(L, "time", luaTime);
}

// CRC

#define LUACRCFUNC(LCF__n, LCF__fn, LCF__o) static int LCF__n(lua_State* L) {\
    CHECKARGS(L, 1, 1 + (LCF__o));\
    size_t l;\
    const char* s = luaL_checklstring(L, 1, &l);\
    lua_pushinteger(L, LCF__fn(s, l));\
    return 1;\
}
#define LUACCRCFUNC(LCF__n, LCF__fn, LCF__o, LCF__ca, LCF__sa) static int LCF__n(lua_State* L) {\
    CHECKARGS(L, 2, 2 + (LCF__o));\
    lua_Integer c = luaL_checkinteger(L, (LCF__ca));\
    size_t l;\
    const char* s = luaL_checklstring(L, (LCF__sa), &l);\
    lua_pushinteger(L, LCF__fn(c, s, l));\
    return 1;\
}
LUACRCFUNC(luaCrc32, crc32, 0);
LUACRCFUNC(luaCrc64, crc64, 0);
LUACRCFUNC(luaICrc32, strncasecrc32, 0);
LUACRCFUNC(luaICrc64, strncasecrc64, 0);
LUACCRCFUNC(luaCCrc32, ccrc32, 0, 1, 2);
LUACCRCFUNC(luaCCrc64, ccrc64, 0, 1, 2);
LUACCRCFUNC(luaCICrc32, cstrncasecrc32, 0, 1, 2);
LUACCRCFUNC(luaCICrc64, cstrncasecrc64, 0, 1, 2);
LUACRCFUNC(luaMCrc32, crc32, -1);
LUACRCFUNC(luaMCrc64, crc64, -1);
LUACRCFUNC(luaMICrc32, strncasecrc32, -1);
LUACRCFUNC(luaMICrc64, strncasecrc64, -1);
LUACCRCFUNC(luaMCCrc32, ccrc32, -1, 2, 1);
LUACCRCFUNC(luaMCCrc64, ccrc64, -1, 2, 1);
LUACCRCFUNC(luaMCICrc32, cstrncasecrc32, -1, 2, 1);
LUACCRCFUNC(luaMCICrc64, cstrncasecrc64, -1, 2, 1);
static inline void setupLuaState_crc(lua_State* L) {
    SETFIELD_FUNC(L, "crc32", luaCrc32);
    SETFIELD_FUNC(L, "crc64", luaCrc64);
    SETFIELD_FUNC(L, "icrc32", luaICrc32);
    SETFIELD_FUNC(L, "icrc64", luaICrc64);
    SETFIELD_FUNC(L, "ccrc32", luaCCrc32);
    SETFIELD_FUNC(L, "ccrc64", luaCCrc64);
    SETFIELD_FUNC(L, "cicrc32", luaCICrc32);
    SETFIELD_FUNC(L, "cicrc64", luaCICrc64);

    lua_getglobal(L, "string");
        SETFIELD_FUNC(L, "crc32", luaMCrc32);
        SETFIELD_FUNC(L, "crc64", luaMCrc64);
        SETFIELD_FUNC(L, "icrc32", luaMICrc32);
        SETFIELD_FUNC(L, "icrc64", luaMICrc64);
        SETFIELD_FUNC(L, "ccrc32", luaMCCrc32);
        SETFIELD_FUNC(L, "ccrc64", luaMCCrc64);
        SETFIELD_FUNC(L, "cicrc32", luaMCICrc32);
        SETFIELD_FUNC(L, "cicrc64", luaMCICrc64);
    lua_pop(L, 1);
}
