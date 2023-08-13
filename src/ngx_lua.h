
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
    ngx_buf_t       *buf;
    int             status;
} ngx_lua_t;

ngx_lua_t *ngx_lua_create(ngx_conf_t *cf);
ngx_lua_t *ngx_lua_clone(ngx_pool_t *pool, ngx_lua_t *from);
ngx_int_t ngx_lua_call(ngx_lua_t *lua, int ref, ngx_log_t *log);

#define ngx_lua_ext_set(L, ext)                                     \
    *((void **) lua_getextraspace(L)) = ext;

#define ngx_lua_ext_get(L)                                          \
    (*((void **) lua_getextraspace(L)))

#endif /* NGX_LUA_H */
