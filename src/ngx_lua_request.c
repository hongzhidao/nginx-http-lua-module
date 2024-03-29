
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_lua_core.h>
#include <ngx_lua_http.h>

static int ngx_lua_request_index(lua_State *L);
static int ngx_lua_request_uri(lua_State *L);
static int ngx_lua_request_method(lua_State *L);
static int ngx_lua_request_client_ip(lua_State *L);
static int ngx_lua_request_body(lua_State *L);
static int ngx_lua_request_arg(lua_State *L);
static int ngx_lua_request_var(lua_State *L);
static int ngx_lua_request_headers(lua_State *L);
static int ngx_lua_request_response(lua_State *L);
static int ngx_lua_request_echo(lua_State *L);
static int ngx_lua_request_exit(lua_State *L);
static int ngx_lua_request_match_cidr(lua_State *L);


void
ngx_lua_request_metatable(lua_State *L)
{
    luaL_newmetatable(L, "lua_request_metatable");

    lua_pushcfunction(L, ngx_lua_request_uri);
    lua_setfield(L, -2, "uri");

    lua_pushcfunction(L, ngx_lua_request_method);
    lua_setfield(L, -2, "method");

    lua_pushcfunction(L, ngx_lua_request_client_ip);
    lua_setfield(L, -2, "client_ip");

    lua_pushcfunction(L, ngx_lua_request_body);
    lua_setfield(L, -2, "body");

    lua_pushcfunction(L, ngx_lua_request_headers);
    lua_setfield(L, -2, "headers");

    lua_pushcfunction(L, ngx_lua_request_response);
    lua_setfield(L, -2, "resp");

    lua_pushcfunction(L, ngx_lua_request_index);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, ngx_lua_request_index);
    lua_setfield(L, -2, "__newindex");

    lua_pop(L, 1);
}


int
ngx_lua_http_request_object(lua_State *L)
{
    lua_newtable(L);

    /* r.args { */
    lua_newtable(L);

    lua_newtable(L);
    lua_pushcfunction(L, ngx_lua_request_arg);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "args");
    /* } r.args */

    /* r.vars { */
    lua_newtable(L);

    lua_newtable(L);
    lua_pushcfunction(L, ngx_lua_request_var);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, ngx_lua_request_var);
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "vars");
    /* } r.vars */

    lua_pushcfunction(L, ngx_lua_request_echo);
    lua_setfield(L, -2, "echo");

    lua_pushcfunction(L, ngx_lua_request_exit);
    lua_setfield(L, -2, "exit");

    lua_pushcfunction(L, ngx_lua_request_match_cidr);
    lua_setfield(L, -2, "match_cidr");

    luaL_setmetatable(L, "lua_request_metatable");

    return luaL_ref(L, LUA_REGISTRYINDEX);
}


static ngx_http_request_t *
ngx_lua_http_request(lua_State *L)
{
    ngx_lua_t  *lua;

    lua = ngx_lua_ext_get(L);

    return lua->data;
}


static int
ngx_lua_request_index(lua_State *L)
{
    int        n;
    ngx_str_t  name;

    n = lua_gettop(L);

    name.data = (u_char *) luaL_checklstring(L, 2, &name.len);

    luaL_getmetatable(L, "lua_request_metatable");

    lua_getfield(L, -1, (const char *) name.data);

    if (!lua_isfunction(L, -1)) {
        lua_pushnil(L);
        return 1;
    }

    if (n == 2) {
        lua_call(L, 0, 1);
        return 1;
    }

    lua_pushvalue(L, 3);
    lua_call(L, 1, 0);

    return 0;
}


static int
ngx_lua_request_uri(lua_State *L)
{
    int                 n;
    ngx_str_t           str, uri;
    ngx_http_request_t  *r;

    r = ngx_lua_http_request(L);
    n = lua_gettop(L);

    if (n == 0) {
        lua_pushlstring(L, (const char *) r->uri.data, r->uri.len);
        return 1;
    }

    str.data = (u_char *) luaL_checklstring(L, 1, &str.len);

    uri.len = str.len;
    uri.data = ngx_pstrdup(r->pool, &str);

    if (uri.data == NULL) {
        luaL_error(L, "uri set failed");
        return 0;
    }

    r->uri = uri;

    return 0;
}


static int
ngx_lua_request_method(lua_State *L)
{
    ngx_http_request_t  *r;

    r = ngx_lua_http_request(L);

    lua_pushlstring(L, (const char *) r->method_name.data, r->method_name.len);

    return 1;
}


static int
ngx_lua_request_client_ip(lua_State *L)
{
    ngx_connection_t    *c;
    ngx_http_request_t  *r;

    r = ngx_lua_http_request(L);
    c = r->connection;

    lua_pushlstring(L, (const char *) c->addr_text.data, c->addr_text.len);

    return 1;
}


static int
ngx_lua_request_body(lua_State *L)
{
    ngx_buf_t           *buf;
    ngx_chain_t         *cl;
    luaL_Buffer         b;
    ngx_http_request_t  *r;

    r = ngx_lua_http_request(L);

    if (r->request_body == NULL
        || r->request_body->bufs == NULL
        || r->request_body->temp_file)
    {
        lua_pushnil(L);
        return 1;
    }

    luaL_buffinit(L, &b);

    cl = r->request_body->bufs;

    while (cl) {
        buf = cl->buf;
        luaL_addlstring(&b, (char *) buf->pos, buf->last - buf->pos);
        cl = cl->next;
    }

    luaL_pushresult(&b);

    return 1;
}


static int
ngx_lua_request_arg(lua_State *L)
{
    ngx_int_t           ret;
    ngx_str_t           name, value;
    ngx_http_request_t  *r;

    r = ngx_lua_http_request(L);

    name.data = (u_char *) luaL_checklstring(L, 2, &name.len);

    if (name.len == 0) {
        return 0;
    }

    ret = ngx_http_arg(r, name.data, name.len, &value);
    if (ret != NGX_OK) {
        return 0;
    }

    lua_pushlstring(L, (const char *) value.data, value.len);

    return 1;
}


static int
ngx_lua_request_var(lua_State *L)
{
    int                        n;
    ngx_str_t                  name, value;
    ngx_uint_t                 key;
    ngx_http_request_t         *r;
    ngx_http_variable_t        *v;
    ngx_http_variable_value_t  *vv;
    ngx_http_core_main_conf_t  *cmcf;

    r = ngx_lua_http_request(L);
    n = lua_gettop(L);

    name.data = (u_char *) luaL_checklstring(L, 2, &name.len);
    key = ngx_hash_strlow(name.data, name.data, name.len);

    if (n == 2) {
        vv = ngx_http_get_variable(r, &name, key);
        if (vv == NULL || vv->not_found) {
            return 0;
        }

        lua_pushlstring(L, (const char *) vv->data, vv->len);
        return 1;
    }

    value.data = (u_char *) luaL_checklstring(L, 3, &value.len);

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    v = ngx_hash_find(&cmcf->variables_hash, key, name.data, name.len);
    if (v == NULL) {
        luaL_error(L, "variable not found");
        return 0;
    }

    if (v->set_handler != NULL) {
        vv = ngx_pcalloc(r->pool, sizeof(ngx_http_variable_value_t));
        if (vv == NULL) {
            goto fail;
        }

        vv->valid = 1;
        vv->not_found = 0;
        vv->data = value.data;
        vv->len = value.len;

        v->set_handler(r, vv, v->data);

        return 0;
    }

    if (!(v->flags & NGX_HTTP_VAR_INDEXED)) {
        luaL_error(L, "variable is not writable");
        return 0;
    }

    vv = &r->variables[v->index];

    vv->valid = 1;
    vv->not_found = 0;

    vv->data = ngx_pnalloc(r->pool, value.len);
    if (vv->data == NULL) {
        goto fail;
    }

    vv->len = value.len;
    ngx_memcpy(vv->data, value.data, vv->len);

    return 0;

fail:

    luaL_error(L, "variable set failed");

    return 0;
}


static int
ngx_lua_request_headers(lua_State *L)
{
    ngx_http_request_t  *r;

    r = ngx_lua_http_request(L);

    lua_pushlightuserdata(L, &r->headers_in.headers);

    luaL_setmetatable(L, "lua_headers_metatable");

    return 1;
}


static int
ngx_lua_request_response(lua_State *L)
{
    ngx_http_request_t  *r;

    r = ngx_lua_http_request(L);

    lua_pushlightuserdata(L, r);

    luaL_setmetatable(L, "lua_response_metatable");

    return 1;
}


static int
ngx_lua_request_echo(lua_State *L)
{
    ngx_buf_t           *b;
    ngx_str_t           str;
    ngx_http_request_t  *r;
    ngx_http_lua_ctx_t  *ctx;

    r = ngx_lua_http_request(L);
    ctx = ngx_http_get_module_ctx(r, ngx_http_lua_module);

    str.data = (u_char *) luaL_checklstring(L, 1, &str.len);

    b = ngx_create_temp_buf(r->pool, str.len);
    if (b == NULL) {
        return luaL_error(L, "echo() failed");
    }

    b->last = ngx_cpymem(b->last, str.data, str.len);

    ctx->buf = b;

    return 0;
}


static int
ngx_lua_request_exit(lua_State *L)
{
    int                 status;
    ngx_http_lua_ctx_t  *ctx;
    ngx_http_request_t  *r;

    r = ngx_lua_http_request(L);
    ctx = ngx_http_get_module_ctx(r, ngx_http_lua_module);

    status = luaL_checkinteger(L, 1);

    if (status < 0 || status > 999) {
        return luaL_error(L, "code is out of range");
    }

    ctx->status = status;

    lua_yield(L, 0);

    return 0;
}


static int
ngx_lua_request_match_cidr(lua_State *L)
{
    int                 matched;
    ngx_cidr_t          *cidr;
    struct sockaddr     *sockaddr;
    ngx_http_request_t  *r;

    r = ngx_lua_http_request(L);
    sockaddr = r->connection->sockaddr;

    cidr = lua_touserdata(L, 1);

    matched = ngx_lua_cidr_match(cidr, sockaddr);

    lua_pushboolean(L, matched);

    return 1;
}
