
/*
 * Copyright (C) Zhidao HONG
 */

#ifndef NGX_LUA_CORE_H
#define NGX_LUA_CORE_H

#include <ngx_lua.h>

void ngx_lua_core_register(lua_State *L);
void ngx_lua_json_register(lua_State *L);

#endif /* NGX_LUA_CORE_H */
