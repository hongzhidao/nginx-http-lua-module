
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_lua_core.h>
#include <ngx_lua_http.h>

/*
 * This dict is based on the njs open-source project's code
 * with some modifications.
 */

typedef struct {
    ngx_lua_dict_t   *dict;
} ngx_lua_dict_data_t;

static int ngx_lua_dict_index(lua_State *L);
static int ngx_lua_dict_get(lua_State *L);
static int ngx_lua_dict_set(lua_State *L);

static const struct luaL_Reg  ngx_lua_dict_methods[] = {
    {"get", ngx_lua_dict_get},
    {"set", ngx_lua_dict_set},
    {NULL, NULL},
};

#define LUA_DICT_META  "dict.meta"

void
ngx_lua_dict_register(lua_State *L)
{
    /*
     * ngx.shared = {};
     * m = {};
     * m.__index = dict_index;
     * setmetatable(ngx.shared, m);
    */
    /* ngx.shared = {} */
    lua_newtable(L);

    lua_newtable(L);

    lua_pushcfunction(L, ngx_lua_dict_index);
    lua_setfield(L, -2, "__index");

    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "shared");

    /*
     * m = {}
     * m.__index = {}
     * m.methods
    */
    luaL_newmetatable(L, LUA_DICT_META);

    lua_pushvalue(L, -1);

    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, ngx_lua_dict_methods, 0);

    lua_pop(L, 1);
}


static ngx_shm_zone_t *
ngx_lua_dict_shm_zone(lua_State *L, ngx_str_t *name)
{
    ngx_uint_t                i;
    ngx_lua_dict_t            *dict;
    ngx_shm_zone_t            *zone;
    ngx_http_lua_main_conf_t  *lmcf;

    lmcf = ngx_http_cycle_get_module_main_conf(ngx_cycle, ngx_http_lua_module);

    dict = lmcf->dicts->elts;

    for (i = 0; i < lmcf->dicts->nelts; i++) {
        zone = dict[i].shm_zone;

        if (zone->shm.name.len == name->len
            && ngx_strncmp(zone->shm.name.data, name->data, name->len)
               == 0)
        {
            return zone;
        }
    }

    return NULL;
}


static int
ngx_lua_dict_index(lua_State *L)
{
    ngx_str_t            name;
    ngx_shm_zone_t       *zone;
    ngx_lua_dict_t       *dict;
    ngx_lua_dict_data_t  *data;

    name.data = (u_char *) luaL_checklstring(L, 2, &name.len);

    zone = ngx_lua_dict_shm_zone(L, &name);
    if (zone == NULL) {
        return 0;
    }

    dict = zone->data;

    data = lua_newuserdata(L, sizeof(ngx_lua_dict_data_t));
    if (data == NULL) {
        return 0;
    }

    data->dict = dict;

    luaL_getmetatable(L, LUA_DICT_META);
    lua_setmetatable(L, -2);

    return 1;
}


static ngx_lua_dict_node_t *
ngx_lua_dict_lookup(ngx_lua_dict_t *dict, ngx_str_t *name)
{
    uint32_t       hash;
    ngx_rbtree_t  *rbtree;

    rbtree = &dict->sh->rbtree;

    hash = ngx_crc32_long(name->data, name->len);

    return (ngx_lua_dict_node_t *) ngx_str_rbtree_lookup(rbtree, name, hash);
}


static int
ngx_lua_dict_get(lua_State *L)
{
    ngx_str_t              name;
    ngx_lua_dict_t         *dict;
    ngx_lua_dict_node_t    *node;
    ngx_lua_dict_data_t    *data;

    data = luaL_checkudata(L, 1, LUA_DICT_META);
    name.data = (u_char *) luaL_checklstring(L, 2, &name.len);

    dict = data->dict;

    ngx_rwlock_rlock(&dict->sh->rwlock);

    node = ngx_lua_dict_lookup(dict, &name);

    if (node == NULL) {
        goto not_found;
    }

    lua_pushlstring(L, (const char *) node->value.data, node->value.len);

    ngx_rwlock_unlock(&dict->sh->rwlock);

    return 1;

not_found:

    ngx_rwlock_unlock(&dict->sh->rwlock);

    return 0;
}


static ngx_int_t
ngx_lua_dict_add(ngx_lua_dict_t *dict, ngx_str_t *name, ngx_str_t *value)
{
    size_t               n;
    ngx_lua_dict_node_t  *node;

    n = sizeof(ngx_lua_dict_node_t) + name->len;

    node = ngx_slab_alloc_locked(dict->shpool, n);
    if (node == NULL) {
        return NGX_ERROR;
    }

    node->sn.str.data = (u_char *) node + sizeof(ngx_lua_dict_node_t);

    node->value.data = ngx_slab_alloc_locked(dict->shpool, value->len);
    if (node->value.data == NULL) {
        ngx_slab_free_locked(dict->shpool, node);
        return NGX_ERROR;
    }

    ngx_memcpy(node->value.data, value->data, value->len);
    node->value.len = value->len;

    ngx_memcpy(node->sn.str.data, name->data, name->len);
    node->sn.str.len = name->len;
    node->sn.node.key = ngx_crc32_long(name->data, name->len);

    ngx_rbtree_insert(&dict->sh->rbtree, &node->sn.node);

    return NGX_OK;
}


static ngx_int_t
ngx_lua_dict_update(ngx_lua_dict_t *dict, ngx_lua_dict_node_t *node,
    ngx_str_t *value)
{
    u_char  *p;

    p = ngx_slab_alloc_locked(dict->shpool, value->len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    ngx_slab_free_locked(dict->shpool, node->value.data);

    node->value.data = p;
    ngx_memcpy(node->value.data, value->data, value->len);

    node->value.len = value->len;

    return NGX_OK;
}


static int
ngx_lua_dict_set(lua_State *L)
{
    ngx_int_t            ret;
    ngx_str_t            name, value;
    ngx_lua_dict_t       *dict;
    ngx_lua_dict_node_t  *node;
    ngx_lua_dict_data_t  *data;

    data = luaL_checkudata(L, 1, LUA_DICT_META);
    dict = data->dict;

    name.data = (u_char *) luaL_checklstring(L, 2, &name.len);
    value.data = (u_char *) luaL_checklstring(L, 3, &value.len);

    ngx_rwlock_wlock(&dict->sh->rwlock);

    node = ngx_lua_dict_lookup(dict, &name);

    if (node == NULL) {
        ret = ngx_lua_dict_add(dict, &name, &value);

    } else {
        ret = ngx_lua_dict_update(dict, node, &value);
    }

    lua_pushboolean(L, (ret != NGX_OK));

    ngx_rwlock_unlock(&dict->sh->rwlock);

    return 1;
}
