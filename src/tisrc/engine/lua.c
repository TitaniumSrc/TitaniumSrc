#include "lua.h"

void regEngineLuaItf(lua_State* L) {
    lua_getglobal(L, "tisrc");

        // TODO

    lua_pop(L, 1);
}

void regClientEntityLuaItf(lua_State* L) {
    lua_getglobal(L, "tisrc");

        // TODO

    lua_pop(L, 1);
}
