#ifndef TISRC_LUA_H
#define TISRC_LUA_H

#include "datastream.h"

#include "../lua/lua.h"

#include <stdbool.h>

lua_State* newLuaState(void);
void delLuaState(lua_State*);

bool runLuaStream(lua_State*, struct datastream*, int nargs, int nresults);

#endif
