
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_lua_http.h>

static int ngx_lua_headers_get(lua_State *L);
static int ngx_lua_headers_set(lua_State *L);


void
ngx_lua_headers_metatable(lua_State *L)
{
    luaL_newmetatable(L, "lua_headers_metatable");

    lua_pushcfunction(L, ngx_lua_headers_get);
    lua_setfield(L, -2, "get");

    lua_pushcfunction(L, ngx_lua_headers_set);
    lua_setfield(L, -2, "set");

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);
}


static ngx_table_elt_t *
ngx_lua_header_find(ngx_list_t *headers, ngx_str_t *name)
{
    ngx_uint_t       i;
    ngx_list_part_t  *part;
    ngx_table_elt_t  *header;

    part = &headers->part;
    header = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            header = part->elts;
            i = 0;
        }

        if (header[i].hash != 0
            && name->len == header[i].key.len
            && ngx_strncasecmp(name->data, header[i].key.data, name->len) == 0)
        {
            return &header[i];
        }
    }

    return NULL;
}


static int
ngx_lua_headers_get(lua_State *L)
{
    ngx_str_t        name;
    ngx_list_t       *headers;
    ngx_table_elt_t  *h;

    headers = lua_touserdata(L, 1);

    name.data = (u_char *) luaL_checklstring(L, 2, &name.len);

    h = ngx_lua_header_find(headers, &name);
    if (h == NULL) {
        return 0;
    }

    lua_pushlstring(L, (const char *) h->value.data, h->value.len);

    return 1;
}


static int
ngx_lua_headers_set(lua_State *L)
{
    ngx_str_t        name, value;
    ngx_list_t       *headers;
    ngx_table_elt_t  *h;

    headers = lua_touserdata(L, 1);

    name.data = (u_char *) luaL_checklstring(L, 2, &name.len);
    value.data = (u_char *) luaL_checklstring(L, 3, &value.len);

    h = ngx_lua_header_find(headers, &name);

    if (h == NULL) {
        h = ngx_list_push(headers);
        if (h == NULL) {
            goto fail;
        }

        h->key.len = name.len;

        h->key.data = ngx_pnalloc(headers->pool, name.len);
        if (h->key.data == NULL) {
            goto fail;
        }

        ngx_memcpy(h->key.data, name.data, name.len);
    }

    h->hash = 1;
    h->value.len = value.len;

    h->value.data = ngx_pnalloc(headers->pool, value.len);
    if (h->value.data == NULL) {
        goto fail;
    }

    ngx_memcpy(h->value.data, value.data, value.len);

    return 0;

fail:

    return luaL_error(L, "headers set failed.");
}
