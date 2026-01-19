#include "lua.h"

#include "datastream.h"
#include "string.h"
#include "logging.h"
#include "version.h"
#include "platform.h"
#include "time.h"
#include "util.h"
#include "crc.h"

static inline void regBaseLuaItf_platforms(lua_State* L);
static inline void regBaseLuaItf_logging(lua_State* L);
static inline void regBaseLuaItf_time(lua_State* L);
static inline void regBaseLuaItf_crc(lua_State* L);
static inline void regBaseLuaItf(lua_State* L) {
    lua_newtable(L);

    lua_newtable(L);
        LI_SETFIELD_INT(L, "build", TISRC_BUILD);
        LI_SETFIELD_INT(L, "platform", PLATFORM);
        LI_SETFIELD_INT(L, "platflags", PLATFLAGS);
        LI_SETFIELD_INT(L, "arch", ARCH);
        LI_SETFIELD_STR(L, "archstr", ARCHSTR);
        LI_SETFIELD_INT(L, "byteorder", BYTEORDER);
    lua_setfield(L, -2, "info");

    regBaseLuaItf_platforms(L);
    regBaseLuaItf_logging(L);
    regBaseLuaItf_time(L);
    regBaseLuaItf_crc(L);

    lua_setglobal(L, "tisrc");
}

lua_State* newLuaState(void) {
    lua_State* L = luaL_newstate();
    if (!L) return NULL;

    luaL_openselectedlibs(L, LUA_GLIBK | LUA_MATHLIBK | LUA_STRLIBK | LUA_TABLIBK | LUA_UTF8LIBK, 0);

    LI_SETGLOBAL_NIL(L, "collectgarbage");
    LI_SETGLOBAL_NIL(L, "dofile");
    LI_SETGLOBAL_NIL(L, "load");
    LI_SETGLOBAL_NIL(L, "loadfile");
    LI_SETGLOBAL_NIL(L, "pcall");
    LI_SETGLOBAL_NIL(L, "print");
    LI_SETGLOBAL_NIL(L, "warn");
    LI_SETGLOBAL_NIL(L, "xpcall");

    regBaseLuaItf(L);

    return L;
}
void delLuaState(lua_State* L) {
    lua_close(L);
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

const char* getLuaFuncName(lua_State* L) {
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

// Logging

static int luaPlog(lua_State* L) {
    LI_CHECKARGS(L, 3, 2);
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
static inline void regBaseLuaItf_logging(lua_State* L) {
    lua_newtable(L);

        lua_newtable(L);
            LI_SETFIELD_FUNC(L, "__call", luaPlog);
        lua_setmetatable(L, -2);

        LI_SETFIELD_FUNC(L, "terry", luaPlogTerry);

        LI_SETFIELD_INT(L, "plain", LL_PLAIN);
        LI_SETFIELD_INT(L, "ms", LL_MS);
        LI_SETFIELD_INT(L, "info", LL_INFO);
        LI_SETFIELD_INT(L, "warn", LL_WARN);
        LI_SETFIELD_INT(L, "error", LL_ERROR);
        LI_SETFIELD_INT(L, "crit", LL_CRIT);

        LI_SETFIELD_INT(L, "func", LF_FUNC);
        LI_SETFIELD_INT(L, "funcln", LF_FUNCLN);
        LI_SETFIELD_INT(L, "msgbox", LF_MSGBOX);
        LI_SETFIELD_INT(L, "debug", LF_DEBUG);

    lua_setfield(L, -2, "log");
}

// Platforms

static inline void regBaseLuaItf_platforms(lua_State* L) {
    lua_newtable(L);
        for (unsigned i = 0; i < PLAT__COUNT; ++i) {
            lua_newtable(L);
                LI_SETFIELD_STR(L, "name", platname[i]);
                LI_SETFIELD_STR(L, "id", platid[i]);
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
            LI_SETFIELD_INT(L, platid[i], i);
        }
    lua_setfield(L, -2, "platform");

    lua_newtable(L);
        LI_SETFIELD_INT(L, "unixlike", PLATFLAG_UNIXLIKE);
        LI_SETFIELD_INT(L, "windowslike", PLATFLAG_WINDOWSLIKE);
        LI_SETFIELD_INT(L, "is64bit", PLATFLAG_64BIT);
    lua_setfield(L, -2, "platflag");

    lua_newtable(L);
        LI_SETFIELD_INT(L, "unknown", ARCH_UNKNOWN);
        LI_SETFIELD_INT(L, "amd64", ARCH_AMD64);
        LI_SETFIELD_INT(L, "arm", ARCH_ARM);
        LI_SETFIELD_INT(L, "arm64", ARCH_ARM64);
        LI_SETFIELD_INT(L, "i386", ARCH_I386);
        LI_SETFIELD_INT(L, "mips", ARCH_MIPS);
        LI_SETFIELD_INT(L, "ppc", ARCH_PPC);
        LI_SETFIELD_INT(L, "wasm", ARCH_WASM);
    lua_setfield(L, -2, "arch");

    lua_newtable(L);
        LI_SETFIELD_INT(L, "little", BO_LE);
        LI_SETFIELD_INT(L, "big", BO_BE);
    lua_setfield(L, -2, "byteorder");
}

// Time

static int luaDelay(lua_State* L) {
    LI_CHECKARGS(L, 1, 1);
    microwait(luaL_checkinteger(L, 1));
    return 0;
}
static int luaTime(lua_State* L) {
    LI_CHECKARGS(L, 0, 0);
    lua_pushinteger(L, altutime());
    return 1;
}
static inline void regBaseLuaItf_time(lua_State* L) {
    LI_SETFIELD_FUNC(L, "delay", luaDelay);
    LI_SETFIELD_FUNC(L, "time", luaTime);
}

// CRC

#define LUACRCFUNC(LCF__n, LCF__fn, LCF__o) static int LCF__n(lua_State* L) {\
    LI_CHECKARGS(L, 1, 1 + (LCF__o));\
    size_t l;\
    const char* s = luaL_checklstring(L, 1, &l);\
    lua_pushinteger(L, LCF__fn(s, l));\
    return 1;\
}
#define LUACCRCFUNC(LCF__n, LCF__fn, LCF__o, LCF__ca, LCF__sa) static int LCF__n(lua_State* L) {\
    LI_CHECKARGS(L, 2, 2 + (LCF__o));\
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
static inline void regBaseLuaItf_crc(lua_State* L) {
    LI_SETFIELD_FUNC(L, "crc32", luaCrc32);
    LI_SETFIELD_FUNC(L, "crc64", luaCrc64);
    LI_SETFIELD_FUNC(L, "icrc32", luaICrc32);
    LI_SETFIELD_FUNC(L, "icrc64", luaICrc64);
    LI_SETFIELD_FUNC(L, "ccrc32", luaCCrc32);
    LI_SETFIELD_FUNC(L, "ccrc64", luaCCrc64);
    LI_SETFIELD_FUNC(L, "cicrc32", luaCICrc32);
    LI_SETFIELD_FUNC(L, "cicrc64", luaCICrc64);

    lua_getglobal(L, "string");
        LI_SETFIELD_FUNC(L, "crc32", luaMCrc32);
        LI_SETFIELD_FUNC(L, "crc64", luaMCrc64);
        LI_SETFIELD_FUNC(L, "icrc32", luaMICrc32);
        LI_SETFIELD_FUNC(L, "icrc64", luaMICrc64);
        LI_SETFIELD_FUNC(L, "ccrc32", luaMCCrc32);
        LI_SETFIELD_FUNC(L, "ccrc64", luaMCCrc64);
        LI_SETFIELD_FUNC(L, "cicrc32", luaMCICrc32);
        LI_SETFIELD_FUNC(L, "cicrc64", luaMCICrc64);
    lua_pop(L, 1);
}
