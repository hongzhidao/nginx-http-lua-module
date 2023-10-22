
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_lua.h>
#include <ngx_event.h>

static void ngx_lua_state_cleanup(void *data);

ngx_lua_t *
ngx_lua_create(ngx_pool_t *pool)
{
    ngx_lua_t           *lua;
    ngx_pool_cleanup_t  *cln;

    lua = ngx_pcalloc(pool, sizeof(ngx_lua_t));
    if (lua == NULL) {
        return NULL;
    }

    lua->state = luaL_newstate();
    if (lua->state == NULL) {
        return NULL;
    }

    luaL_openlibs(lua->state);

    cln = ngx_pool_cleanup_add(pool, 0);
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
ngx_lua_clone(ngx_lua_t *from)
{
    ngx_lua_t  *lua;
    lua_State  *coro;

    coro = lua_newthread(from->state);
    if (coro == NULL) {
        return NULL;
    }

    lua = lua_newuserdata(coro, sizeof(ngx_lua_t));
    if (lua == NULL) {
        return NULL;
    }

    ngx_memzero(lua, sizeof(ngx_lua_t));

    lua->state = coro;

    lua->ref = luaL_ref(from->state, LUA_REGISTRYINDEX);

    ngx_lua_ext_set(lua->state, lua);

    return lua;
}


void
ngx_lua_free(ngx_lua_t *from, ngx_lua_t *lua)
{
    luaL_unref(from->state, LUA_REGISTRYINDEX, lua->ref);
}


ngx_int_t
ngx_lua_call(ngx_lua_t *lua, int nargs, ngx_event_t *wake)
{
    int  status, nresults;

    if (lua->wake != NULL) {
        nargs = lua->nresults;
    }

    status = lua_resume(lua->state, NULL, nargs, &nresults);

    switch (status) {

    case LUA_YIELD:
        lua->wake = wake;
        return NGX_AGAIN;

    case LUA_OK:
        lua->wake = NULL;
        return NGX_OK;

    default:
        ngx_log_error(NGX_LOG_ERR, lua->log, 0, "lua exception: %s",
                      lua_tostring(lua->state, -1));
        return NGX_ERROR;
    }

    return NGX_OK;
}


int
ngx_lua_yield(ngx_lua_t *lua)
{
    return lua_yield(lua->state, 0);
}


void
ngx_lua_wake(ngx_lua_t *lua, int nresults)
{
    lua->nresults = nresults;

    ngx_post_event(lua->wake, &ngx_posted_events);
}
