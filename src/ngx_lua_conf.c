
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_lua_core.h>

static int ngx_lua_conf_index(lua_State *L);
static int ngx_lua_conf_data(lua_State *L);
static int ngx_lua_conf_free(lua_State *L);


static void
ngx_lua_conf_metatable(lua_State *L)
{
    luaL_newmetatable(L, "lua_conf_metatable");

    lua_pushcfunction(L, ngx_lua_conf_index);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, ngx_lua_conf_data);
    lua_setfield(L, -2, "data");

    lua_pushcfunction(L, ngx_lua_conf_free);
    lua_setfield(L, -2, "__gc");

    lua_pop(L, 1);
}


ngx_lua_conf_t *
ngx_lua_conf_new(lua_State *L, ngx_pool_t *pool)
{
    ngx_lua_conf_t  *conf;

    conf = lua_newuserdata(L, sizeof(ngx_lua_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->pool = pool;

    lua_newtable(L);
    conf->data = luaL_ref(L, LUA_REGISTRYINDEX);

    luaL_getmetatable(L, "lua_conf_metatable");
    lua_setmetatable(L, -2);

    return conf;
}


static int
ngx_lua_conf_free(lua_State *L)
{
    ngx_lua_conf_t  *conf;

    conf = lua_touserdata(L, 1);

    luaL_unref(L, LUA_REGISTRYINDEX, conf->data);

    ngx_destroy_pool(conf->pool);

    return 0;
}


static int
ngx_lua_conf_index(lua_State *L)
{
    ngx_str_t       name;
    ngx_lua_conf_t  *conf;

    conf = lua_touserdata(L, 1);
    name.data = (u_char *) luaL_checklstring(L, 2, &name.len);

    luaL_getmetatable(L, "lua_conf_metatable");
    lua_getfield(L, -1, (const char *) name.data);

    if (!lua_isfunction(L, -1)) {
        return 0;
    }

    lua_pushlightuserdata(L, conf);

    lua_call(L, 1, 1);

    return 1;
}


static int
ngx_lua_conf_data(lua_State *L)
{
    ngx_lua_conf_t  *conf;

    conf = lua_touserdata(L, 1);

    lua_rawgeti(L, LUA_REGISTRYINDEX, conf->data);

    return 1;
}


void
ngx_lua_conf_register(lua_State *L)
{
    ngx_lua_conf_metatable(L);
}
