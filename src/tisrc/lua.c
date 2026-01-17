//#define LUA_LIB

#include "lua.h"

#include "../lua/lua.h"
#include "../lua/lualib.h"
#include "../lua/lauxlib.h"

#include "datastream.h"
#include "string.h"
#include "logging.h"

static int luaPlog(lua_State* L) {
    int lvl = luaL_checkinteger(L, 2);
    const char* msg = luaL_checkstring(L, 3);

    lua_Debug ar;
    if (lua_getstack(L, 1, &ar)) {
        lua_getinfo(L, "nSl", &ar);
        plog_raw(lvl, (ar.name) ? ar.name : "???", ar.short_src, ar.currentline, "%s", msg);
    } else {
        plog(lvl, "%s", msg);
    }
    return 0;
}
static int luaPlogTerry(lua_State* L) {
    plog_raw(1, "lua", "lua", 0, "%s", "Made by the will of Terry!");
    return 0;
}

#define SETFIELD_INT(SETFIELD__L, SETFIELD__name, SETFIELD__value) do {\
    lua_pushinteger((SETFIELD__L), (SETFIELD__value));\
    lua_setfield((SETFIELD__L), -2, (SETFIELD__name));\
} while (0)
#define SETFIELD_LITSTR(SETFIELD__L, SETFIELD__name, SETFIELD__value) do {\
    lua_pushliteral((SETFIELD__L), (SETFIELD__value));\
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
#define OPENLIB(OPENLIB__L, OPENLIB__name, OPENLIB__func) do {\
    lua_pushcfunction((OPENLIB__L), (OPENLIB__func));\
    lua_pushstring((OPENLIB__L), (OPENLIB__name));\
    lua_call(L, 1, 0);\
} while (0)

static inline void setupLuaState_logging(lua_State* L) {
    lua_newtable(L);

        lua_newtable(L);
            SETFIELD_FUNC(L, "__call", luaPlog);
        lua_setmetatable(L, -2);

        SETFIELD_FUNC(L, "terry", luaPlogTerry);

        lua_newtable(L);
            SETFIELD_INT(L, "plain", LL_PLAIN);
            SETFIELD_INT(L, "ms", LL_MS);
            SETFIELD_INT(L, "info", LL_INFO);
            SETFIELD_INT(L, "warn", LL_WARN);
            SETFIELD_INT(L, "error", LL_ERROR);
            SETFIELD_INT(L, "crit", LL_CRIT);
        lua_setfield(L, -2, "level");

        lua_newtable(L);
            SETFIELD_INT(L, "func", LF_FUNC);
            SETFIELD_INT(L, "funcln", LF_FUNCLN);
            SETFIELD_INT(L, "msgbox", LF_MSGBOX);
            SETFIELD_INT(L, "debug", LF_DEBUG);
        lua_setfield(L, -2, "flag");

    lua_setfield(L, -2, "log");
}
static inline void setupLuaState(lua_State* L) {
    lua_newtable(L);
    setupLuaState_logging(L);
    lua_setglobal(L, "tisrc");
}

lua_State* newLuaState(void) {
    lua_State* L = luaL_newstate();
    if (!L) return NULL;

    OPENLIB(L, LUA_GNAME, luaopen_base);

    SETGLOBAL_NIL(L, "collectgarbage");
    SETGLOBAL_NIL(L, "dofile");
    SETGLOBAL_NIL(L, "load");
    SETGLOBAL_NIL(L, "loadfile");
    SETGLOBAL_NIL(L, "pcall");
    SETGLOBAL_NIL(L, "print");
    SETGLOBAL_NIL(L, "warn");
    SETGLOBAL_NIL(L, "xpcall");

    OPENLIB(L, LUA_MATHLIBNAME, luaopen_math);
    OPENLIB(L, LUA_STRLIBNAME, luaopen_string);
    OPENLIB(L, LUA_TABLIBNAME, luaopen_table);
    OPENLIB(L, LUA_UTF8LIBNAME, luaopen_utf8);

    setupLuaState(L);

    return L;
}
void delLuaState(lua_State* L) {
    lua_close(L);
}

const char* luaRunStream_read(lua_State* L, void* data, size_t* size) {
    (void)L;
    return ds_readchunk(data, size);
}
bool runLuaStream(lua_State* L, struct datastream* ds, int nargs, int nresults) {
    struct charbuf cb;
    if (!cb_init(&cb, 128)) return false;
    if (!cb_add(&cb, '=') || !cb_addstr(&cb, ds->name) || !cb_finalize(&cb)) {
        cb_dump(&cb);
        return false;
    }
    if (lua_load(L, luaRunStream_read, ds, cb.data, NULL) || lua_pcall(L, nargs, nresults, 0)) {
        plog(LL_CRIT, "Lua error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        cb_dump(&cb);
        return false;
    }
    cb_dump(&cb);
    return true;
}
