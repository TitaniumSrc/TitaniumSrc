#define LUA_LIB

#include <stdio.h>
#include "luasrc/lua.h"
#include "luasrc/lualib.h"
#include "luasrc/lauxlib.h"
#include "../tisrc/logging.h"

static const luaL_Reg lualibs[] = {
    {LUA_TABLIBNAME, luaopen_table},
    {LUA_STRLIBNAME, luaopen_string},
    {LUA_MATHLIBNAME, luaopen_math},
    {NULL, NULL}
};

LUALIB_API void ti_openlualibs (lua_State *L) {
    const luaL_Reg *lib = lualibs;
    for (; lib->func; lib++) {
        lua_pushcfunction(L, lib->func);
        lua_pushstring(L, lib->name);
        lua_call(L, 1, 0);
    }
}

static int luaPlog(lua_State *L)
{
    int lvl = luaL_checkinteger(L, 2);
    const char *msg = luaL_checkstring(L, 3);

    plog_raw(lvl, "lua", "lua", 0, "%s", msg);
    return 0;
}

static int luaPlogTerry(lua_State *L)
{
    plog_raw(1, "lua", "lua", 0, "%s", "Made by the will of Terry!");
    return 0;
}

void regLua(lua_State *L)
{
    lua_newtable(L);
    lua_newtable(L);

    lua_pushcfunction(L, luaPlog);
    lua_setfield(L, -2, "__call");

    lua_newtable(L);
    lua_pushcfunction(L, luaPlogTerry);
    lua_setfield(L, -2, "terry");
    lua_setfield(L, -2, "__index");

    lua_setmetatable(L, -2);

    lua_newtable(L);
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, "log");

    lua_setglobal(L, "tisrc");
    lua_pop(L, 1);
}



int luaRun(const char *filename)
{
    lua_State *L = luaL_newstate();
    //luaL_openlibs(L);
    ti_openlualibs(L);

    regLua(L);

    if (luaL_dofile(L, filename) != LUA_OK) {
        fprintf(stderr, "Lua error: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        lua_close(L);
        return 0;
    }

    lua_close(L);
    return 1;

}
