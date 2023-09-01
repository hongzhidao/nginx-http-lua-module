
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_lua_core.h>


void
ngx_lua_core_register(lua_State *L)
{
    lua_newtable(L);

    ngx_lua_json_register(L);
    ngx_lua_dict_register(L);
    ngx_lua_cidr_register(L);

    lua_setglobal(L, "ngx");
}
