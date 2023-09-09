
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_lua_core.h>

static void ngx_lua_base64_register(lua_State *L);


void
ngx_lua_core_register(lua_State *L)
{
    lua_newtable(L);

    ngx_lua_json_register(L);
    ngx_lua_dict_register(L);
    ngx_lua_cidr_register(L);
    ngx_lua_base64_register(L);

    lua_setglobal(L, "ngx");
}


static int
ngx_lua_base64_encode(lua_State *L)
{
    size_t     len;
    ngx_str_t  str, encoded;
    ngx_lua_t  *lua;

    lua = ngx_lua_ext_get(L);

    str.data = (u_char *) luaL_checklstring(L, 1, &str.len);

    len = ngx_base64_encoded_length(str.len);

    encoded.data = ngx_pcalloc(lua->pool, len);
    if (encoded.data == NULL) {
        return luaL_error(L, "base64 encode failed");
    }

    ngx_encode_base64(&encoded, &str);

    lua_pushlstring(L, (const char *) encoded.data, encoded.len);

    return 1;
}


static int
ngx_lua_base64_decode(lua_State *L)
{
    size_t     len;
    ngx_int_t  ret;
    ngx_str_t  str, decoded;
    ngx_lua_t  *lua;

    lua = ngx_lua_ext_get(L);

    str.data = (u_char *) luaL_checklstring(L, 1, &str.len);

    len = ngx_base64_decoded_length(str.len);

    decoded.data = ngx_pcalloc(lua->pool, len);
    if (decoded.data == NULL) {
        return luaL_error(L, "base64 decode failed");
    }

    ret = ngx_decode_base64(&decoded, &str);
    if (ret != NGX_OK) {
        return 0;
    }

    lua_pushlstring(L, (const char *) decoded.data, decoded.len);

    return 1;
}


static const struct luaL_Reg  lua_base64_methods[] = {
    {"base64_encode", ngx_lua_base64_encode},
    {"base64_decode", ngx_lua_base64_decode},
    {NULL, NULL},
};


static void
ngx_lua_base64_register(lua_State *L)
{
    luaL_setfuncs(L, lua_base64_methods, 0);
}
