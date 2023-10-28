
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
ngx_lua_conf_new(ngx_lua_t *lua, ngx_log_t *log)
{
    lua_State       *L;
    ngx_pool_t      *pool;
    ngx_lua_conf_t  *conf;

    L = lua->state;

    conf = lua_newuserdata(L, sizeof(ngx_lua_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    luaL_setmetatable(L, "lua_conf_metatable");
    conf->conf_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_newtable(L);
    conf->data_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    pool = ngx_create_pool(4096, log);
    if (pool == NULL) {
        return NULL;
    }

    conf->lua = ngx_lua_clone(lua, pool);
    if (conf->lua == NULL) {
        return NULL;
    }

    conf->lua->log = log;

    return conf;
}


static int
ngx_lua_conf_free(lua_State *L)
{
    ngx_lua_conf_t  *conf;

    conf = lua_touserdata(L, 1);

    ngx_destroy_pool(conf->pool);
    luaL_unref(L, LUA_REGISTRYINDEX, conf->data_ref);
    ngx_lua_free(L, conf->lua);

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

    lua_rawgeti(L, LUA_REGISTRYINDEX, conf->data_ref);

    return 1;
}


void
ngx_lua_conf_register(lua_State *L)
{
    ngx_lua_conf_metatable(L);
}
