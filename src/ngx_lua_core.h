
/*
 * Copyright (C) Zhidao HONG
 */

#ifndef NGX_LUA_CORE_H
#define NGX_LUA_CORE_H

#include <ngx_lua.h>

typedef struct {
    ngx_str_node_t          sn;
    ngx_str_t               value;
} ngx_lua_dict_node_t;

typedef struct {
    ngx_rbtree_t            rbtree;
    ngx_rbtree_node_t       sentinel;
    ngx_atomic_t            rwlock;
} ngx_lua_dict_sh_t;

typedef struct {
    ngx_shm_zone_t          *shm_zone;
    ngx_lua_dict_sh_t       *sh;
    ngx_slab_pool_t         *shpool;
} ngx_lua_dict_t;

void ngx_lua_core_register(lua_State *L);
void ngx_lua_json_register(lua_State *L);
void ngx_lua_dict_register(lua_State *L);

#endif /* NGX_LUA_CORE_H */
