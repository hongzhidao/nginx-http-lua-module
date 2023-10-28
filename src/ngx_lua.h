
/*
 * Copyright (C) Zhidao HONG
 */

#ifndef NGX_LUA_H
#define NGX_LUA_H

#include <ngx_config.h>
#include <ngx_core.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

typedef struct {
    lua_State       *state;
    int             ref;
    void            *data;
    ngx_pool_t      *pool;
    ngx_log_t       *log;
    ngx_event_t     *wake;
    int             nresults;
} ngx_lua_t;

ngx_lua_t *ngx_lua_create(ngx_pool_t *pool);
ngx_lua_t *ngx_lua_clone(ngx_lua_t *from, ngx_pool_t *pool);
void ngx_lua_free(lua_State *L, ngx_lua_t *lua);
ngx_int_t ngx_lua_call(ngx_lua_t *lua, int nargs, ngx_event_t *wake);
int ngx_lua_yield(ngx_lua_t *lua);
void ngx_lua_wake(ngx_lua_t *lua, int nresults);

#define ngx_lua_ext_set(L, ext)                                     \
    *((void **) lua_getextraspace(L)) = ext;

#define ngx_lua_ext_get(L)                                          \
    (*((void **) lua_getextraspace(L)))

#endif /* NGX_LUA_H */
