
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_lua_http.h>

static int ngx_lua_response_index(lua_State *L);
static int ngx_lua_response_headers(lua_State *L);


void
ngx_lua_response_metatable(lua_State *L)
{
    luaL_newmetatable(L, "lua_response_metatable");

    lua_pushcfunction(L, ngx_lua_response_headers);
    lua_setfield(L, -2, "headers");

    lua_pushcfunction(L, ngx_lua_response_index);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, ngx_lua_response_index);
    lua_setfield(L, -2, "__newindex");

    lua_pop(L, 1);
}


static int
ngx_lua_response_index(lua_State *L)
{
    int                 n;
    ngx_str_t           name;
    ngx_http_request_t  *r;

    n = lua_gettop(L);
    r = lua_touserdata(L, 1);
    name.data = (u_char *) luaL_checklstring(L, 2, &name.len);

    luaL_getmetatable(L, "lua_response_metatable");
    lua_getfield(L, -1, (const char *) name.data);

    if (!lua_isfunction(L, -1)) {
        return 0;
    }

    lua_pushlightuserdata(L, r);

    if (n == 2) {
        lua_call(L, 1, 1);
        return 1;
    }

    lua_pushvalue(L, 3);
    lua_call(L, 2, 0);

    return 0;
}


static int
ngx_lua_response_headers(lua_State *L)
{
    ngx_http_request_t  *r;

    r = lua_touserdata(L, 1);

    lua_pushlightuserdata(L, &r->headers_out.headers);

    luaL_setmetatable(L, "lua_headers_metatable");

    return 1;
}
