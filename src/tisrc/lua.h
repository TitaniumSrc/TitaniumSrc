#ifndef TISRC_LUA_H
#define TISRC_LUA_H

#include "datastream.h"

#include "../lua/lua.h"
#include "../lua/lualib.h"
#include "../lua/lauxlib.h"

#include <stdbool.h>

lua_State* newLuaState(void);
void delLuaState(lua_State*);

bool runLuaStream(lua_State*, struct datastream*, int nargs, int nresults);

const char* getLuaFuncName(lua_State* L);

#define LI_ERR_WANTARGS(ERR__L, ERR__n) luaL_error((ERR__L), "expected %d arguments to '%s'", (ERR__n), getLuaFuncName((ERR__L)))

#define LI_CHECKARGS(LI__L, LI__n1, LI__n2) do {\
    if (lua_gettop((LI__L)) != (LI__n1)) return LI_ERR_WANTARGS((LI__L), (LI__n2));\
} while (0)

#define LI_SETFIELD_INT(LI__L, LI__name, LI__value) do {\
    lua_pushinteger((LI__L), (LI__value));\
    lua_setfield((LI__L), -2, (LI__name));\
} while (0)
#define LI_SETFIELD_STR(LI__L, LI__name, LI__value) do {\
    lua_pushstring((LI__L), (LI__value));\
    lua_setfield((LI__L), -2, (LI__name));\
} while (0)
#define LI_SETFIELD_FUNC(LI__L, LI__name, LI__value) do {\
    lua_pushcfunction((LI__L), (LI__value));\
    lua_setfield((LI__L), -2, (LI__name));\
} while (0)
#define LI_SETGLOBAL_NIL(LI__L, LI__name) do {\
    lua_pushnil((LI__L));\
    lua_setglobal((LI__L), (LI__name));\
} while (0)

#endif
