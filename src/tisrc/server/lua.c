#include "lua.h"

void regServerLuaItf(lua_State* L) {
    lua_getglobal(L, "tisrc");

        // TODO

    lua_pop(L, 1);
}

void regEntityLuaItf(lua_State* L) {
    lua_getglobal(L, "tisrc");

        // TODO

    lua_pop(L, 1);
}
