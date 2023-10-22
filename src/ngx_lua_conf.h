
/*
 * Copyright (C) Zhidao HONG
 */

#ifndef NGX_LUA_CONF_H
#define NGX_LUA_CONF_H

typedef struct {
    ngx_pool_t      *pool;
    int             data;
} ngx_lua_conf_t;

ngx_lua_conf_t *ngx_lua_conf_new(lua_State *L, ngx_pool_t *pool);

#endif /* NGX_LUA_CONF_H */
