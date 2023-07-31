
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_lua.h>

static int ngx_lua_http_index(lua_State *L);
static int ngx_lua_http_uri(lua_State *L);
static int ngx_lua_http_echo(lua_State *L);
static int ngx_lua_http_exit(lua_State *L);


void
ngx_lua_http_register(lua_State *L)
{
    /* http property { */
    lua_createtable(L, 0, 4);

    lua_pushcfunction(L, ngx_lua_http_uri);
    lua_setfield(L, -2, "uri");

    lua_setglobal(L, "http_property");
    /* } http property */

    lua_createtable(L, 0, 10);

    lua_pushcfunction(L, ngx_lua_http_echo);
    lua_setfield(L, -2, "echo");

    lua_pushcfunction(L, ngx_lua_http_exit);
    lua_setfield(L, -2, "exit");

    /* setmetatable { */
    lua_createtable(L, 0, 4);

    lua_pushcfunction(L, ngx_lua_http_index);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, ngx_lua_http_index);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2);
    /* } setmetatable */

    lua_setglobal(L, "r");
}


static ngx_http_request_t *
ngx_lua_http_request(lua_State *L)
{
    ngx_lua_t  *lua;

    lua = ngx_lua_ext_get(L);

    return lua->data;
}


static int
ngx_lua_http_index(lua_State *L)
{
    int        n;
    ngx_str_t  name;

    n = lua_gettop(L);

    name.data = (u_char *) luaL_checklstring(L, 2, &name.len);

    lua_getglobal(L, "http_property");

    lua_getfield(L, -1, (const char *) name.data);

    if (!lua_isfunction(L, -1)) {
        lua_pushnil(L);
        return 1;
    }

    if (n == 2) {
        lua_call(L, 0, 1);
        return 1;
    }

    lua_pushvalue(L, -3);
    lua_call(L, 1, 1);

    return 0;
}


static int
ngx_lua_http_uri(lua_State *L)
{
    ngx_http_request_t  *r;

    r = ngx_lua_http_request(L);

    lua_pushlstring(L, (const char *) r->uri.data, r->uri.len);

    return 1;
}


static int
ngx_lua_http_echo(lua_State *L)
{
    ngx_buf_t           *b;
    ngx_str_t           str;
    ngx_lua_t           *lua;
    ngx_http_request_t  *r;

    lua = ngx_lua_ext_get(L);
    r = ngx_lua_http_request(L);

    str.data = (u_char *) luaL_checklstring(L, 1, &str.len);

    b = ngx_create_temp_buf(r->pool, str.len);
    if (b == NULL) {
        return luaL_error(L, "echo() failed");
    }

    b->last = ngx_cpymem(b->last, str.data, str.len);

    lua->buf = b;

    return 0;
}


static int
ngx_lua_http_exit(lua_State *L)
{
    int        status;
    ngx_lua_t  *lua;

    lua = ngx_lua_ext_get(L);

    status = luaL_checkinteger(L, 1);

    if (status < 0 || status > 999) {
        return luaL_error(L, "code is out of range");
    }

    lua->status = status;

    lua_yield(L, 0);

    return 0;
}
