
/*
 * Copyright (C) Zhidao HONG
 */

#ifndef NGX_LUA_HTTP_H
#define NGX_LUA_HTTP_H

#include <ngx_lua.h>
#include <ngx_http.h>

typedef struct {
    ngx_lua_t       *lua;
    int             request_ref;
    ngx_array_t     *dicts;   /* of ngx_lua_dict_t */
    ngx_array_t     *timers;  /* of ngx_lua_timer_t */
} ngx_http_lua_main_conf_t;

typedef struct {
    ngx_uint_t      status;
    ngx_buf_t       *buf;
} ngx_http_lua_ctx_t;

void ngx_lua_request_metatable(lua_State *L);
int ngx_lua_http_request_object(lua_State *L);

extern ngx_module_t  ngx_http_lua_module;

#endif /* NGX_LUA_HTTP_H */
