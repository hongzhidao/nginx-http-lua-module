
/*
 * Copyright (C) Zhidao HONG
 */

#ifndef NGX_LUA_CORE_H
#define NGX_LUA_CORE_H

#include <ngx_lua.h>
#include <ngx_lua_dict.h>

typedef struct {
    ngx_pool_t      *pool;
} ngx_lua_conf_t;

void ngx_lua_nginx_register(lua_State *L);
void ngx_lua_conf_register(lua_State *L);
void ngx_lua_json_register(lua_State *L);
void ngx_lua_dict_register(lua_State *L);
void ngx_lua_cidr_register(lua_State *L);
int ngx_lua_cidr_match(ngx_cidr_t *cidr, struct sockaddr *sockaddr);

#endif /* NGX_LUA_CORE_H */
