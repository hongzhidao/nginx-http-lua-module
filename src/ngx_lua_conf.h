
/*
 * Copyright (C) Zhidao HONG
 */

#ifndef NGX_LUA_CONF_H
#define NGX_LUA_CONF_H

typedef struct {
    ngx_pool_t      *pool;
    ngx_lua_t       *lua;
    int             conf_ref;
    int             data_ref;
} ngx_lua_conf_t;

ngx_lua_conf_t *ngx_lua_conf_new(ngx_lua_t *lua, ngx_log_t *log);

#endif /* NGX_LUA_CONF_H */
