
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_lua.h>

typedef struct {
    ngx_lua_t       *lua;
    ngx_lua_t       *from;
} ngx_lua_cleanup_t;

static void ngx_lua_state_cleanup(void *data);
static void ngx_lua_thread_cleanup(void *data);

ngx_lua_t *
ngx_lua_create(ngx_conf_t *cf)
{
    ngx_lua_t           *lua;
    ngx_pool_cleanup_t  *cln;

    lua = ngx_pcalloc(cf->pool, sizeof(ngx_lua_t));
    if (lua == NULL) {
        return NULL;
    }

    lua->state = luaL_newstate();
    if (lua->state == NULL) {
        return NULL;
    }

    luaL_openlibs(lua->state);

    cln = ngx_pool_cleanup_add(cf->pool, 0);
    if (cln == NULL) {
        lua_close(lua->state);
        return NULL;
    }

    cln->handler = ngx_lua_state_cleanup;
    cln->data = lua;

    return lua;
}


static void
ngx_lua_state_cleanup(void *data)
{
    ngx_lua_t  *lua = data;

    lua_close(lua->state);
}


ngx_lua_t *
ngx_lua_clone(ngx_pool_t *pool, ngx_lua_t *from)
{
    ngx_lua_t           *lua;
    ngx_lua_cleanup_t   *lcln;
    ngx_pool_cleanup_t  *cln;

    lua = ngx_pcalloc(pool, sizeof(ngx_lua_t));
    if (lua == NULL) {
        return NULL;
    }

    lua->pool = pool;

    lua->state = lua_newthread(from->state);
    if (lua->state == NULL) {
        return NULL;
    }

    lua->ref = luaL_ref(from->state, LUA_REGISTRYINDEX);

    ngx_lua_ext_set(lua->state, lua);

    cln = ngx_pool_cleanup_add(pool, sizeof(ngx_lua_cleanup_t));
    if (cln == NULL) {
        luaL_unref(from->state, LUA_REGISTRYINDEX, lua->ref);
        return NULL;
    }

    cln->handler = ngx_lua_thread_cleanup;

    lcln = cln->data;
    lcln->from = from;
    lcln->lua = lua;

    return lua;
}


static void
ngx_lua_thread_cleanup(void *data)
{
    ngx_lua_cleanup_t  *lcln = data;

    luaL_unref(lcln->from->state, LUA_REGISTRYINDEX, lcln->lua->ref);
}


ngx_int_t
ngx_lua_call(ngx_lua_t *lua, int narg)
{
    int  status, nresults;

    status = lua_resume(lua->state, NULL, narg, &nresults);

    if (status != LUA_OK && status != LUA_YIELD) {
        ngx_log_error(NGX_LOG_ERR, lua->log, 0, "lua exception: %s",
                      lua_tostring(lua->state, -1));
        return NGX_ERROR;
    }

    return NGX_OK;
}
