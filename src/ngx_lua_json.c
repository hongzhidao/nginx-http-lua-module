
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_chb.h>
#include <ngx_lua.h>

typedef struct lua_json_builder_s  lua_json_builder_t;
typedef struct lua_json_parser_s   lua_json_parser_t;

static int lua_json_value_append(lua_State *L, lua_json_builder_t *json);
static int lua_json_number_append(lua_State *L, lua_json_builder_t *json,
    int is_key);
static int lua_json_string_append(lua_State *L, lua_json_builder_t *json,
    int is_key);
static int lua_json_array_length(lua_State *L);
static int lua_json_array_append(lua_State *L, int len,
    lua_json_builder_t *json);
static int lua_json_object_append(lua_State *L, lua_json_builder_t *json);
static ngx_inline int lua_json_build_error(lua_json_builder_t *json,
    const char *err);
static int lua_json_next_token(lua_State *L, lua_json_parser_t *json);
static int lua_json_value_parse(lua_State *L, lua_json_parser_t *json);
static int lua_json_next_number(lua_State *L, lua_json_parser_t *json,
    u_char *p, u_char **pp);
static int lua_json_next_string(lua_State *L, lua_json_parser_t *json,
    u_char *p, u_char **pp);
static int lua_json_string_parse(lua_State *L, lua_json_parser_t *json);
static int lua_json_array_parse(lua_State *L, lua_json_parser_t *json);
static int lua_json_object_parse(lua_State *L, lua_json_parser_t *json);
static ngx_inline int lua_json_parse_error(lua_State *L,
    lua_json_parser_t *json, const char *fmt, ...);


struct lua_json_builder_s {
    ngx_chb_t       chb;
    const char      *error;
};

static int
lua_json_encode(lua_State *L)
{
    ngx_int_t           ret;
    ngx_str_t           str;
    ngx_lua_t           *lua;
    lua_json_builder_t  json;

    lua = ngx_lua_ext_get(L);

    ngx_chb_init(&json.chb, lua->pool);

    if (lua_json_value_append(L, &json)) {
        goto fail;
    }

    ret = ngx_chb_join(&json.chb, &str);
    if (ret != NGX_OK) {
        return luaL_error(L, "json encode failed"); 
    }

    lua_pushlstring(L, (const char *) str.data, str.len);
    
    return 1;

fail:

    lua_pushnil(L);
    lua_pushstring(L, json.error);
    
    return 2;
}


static int
lua_json_value_append(lua_State *L, lua_json_builder_t *json)
{
    int  len;

    switch (lua_type(L, -1)) {

    case LUA_TNIL:
        ngx_chb_add(&json->chb, "null");
        break;

    case LUA_TBOOLEAN:
        if (lua_toboolean(L, -1)) {
            ngx_chb_add(&json->chb, "true");

        } else {
            ngx_chb_add(&json->chb, "false");
        }

        break;

    case LUA_TNUMBER:
        return lua_json_number_append(L, json, 0);

    case LUA_TSTRING:
        return lua_json_string_append(L, json, 0);

    case LUA_TTABLE:
        len = lua_json_array_length(L);

        if (len > 0) {
            return lua_json_array_append(L, len, json);

        } else {
            return lua_json_object_append(L, json);
        }

    default:
        return lua_json_build_error(json, "type not supported");
    }

    return 0;
}


static int
lua_json_number_append(lua_State *L, lua_json_builder_t *json, int is_key)
{
    int         len, index;
    double      num;
    u_char      str[35];
    const char  *format;

    index = is_key ? -2 : -1;
    num = lua_tonumber(L, index);

    format = is_key ? "\"%.14g\":" : "%.14g";
    len = snprintf((char *) str, 35, format, num);

    ngx_chb_add_string(&json->chb, str, len);

    return 0;
}


static const char *lua_escape_chars[256] = {
    "\\u0000", "\\u0001", "\\u0002", "\\u0003",
    "\\u0004", "\\u0005", "\\u0006", "\\u0007",
    "\\b",     "\\t",     "\\n",     "\\u000b",
    "\\f",     "\\r",     "\\u000e", "\\u000f",
    "\\u0010", "\\u0011", "\\u0012", "\\u0013",
    "\\u0014", "\\u0015", "\\u0016", "\\u0017",
    "\\u0018", "\\u0019", "\\u001a", "\\u001b",
    "\\u001c", "\\u001d", "\\u001e", "\\u001f",
    NULL, NULL, "\\\"", NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, "\\/",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, "\\\\", NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, "\\u007f",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};


static int
lua_json_string_append(lua_State *L, lua_json_builder_t *json, int is_key)
{
    int         index;
    u_char      *p;
    size_t      i, len, reserve;
    u_char      c, *str;
    const char  *escstr;

    index = is_key ? -2 : -1;
    str = (u_char *) lua_tolstring(L, index, &len);

    reserve = len * 6 + 2 + is_key;
    p = ngx_chb_reserve(&json->chb, reserve);
    if (p == NULL) {
        return luaL_error(L, "json string append failed");
    }

    *p++ = '"';

    for (i = 0; i < len; i++) {
        c = str[i];
        escstr = lua_escape_chars[c];

        if (escstr != NULL) {
            p = ngx_cpymem(p, escstr, strlen(escstr));

        } else {
            *p++ = c;
        }
    }

    *p++ = '"';

    if (is_key) {
        *p++ = ':';
    }

    ngx_chb_advance(&json->chb, p);

    return 0;
}


static int
lua_json_array_length(lua_State *L)
{
    int  n;

    n = 0;

    lua_pushnil(L);

    while (lua_next(L, -2) != 0) {
        n++;

        if (lua_type(L, -2) == LUA_TNUMBER
            && lua_tonumber(L, -2) == n)
        {
            lua_pop(L, 1);
            continue;
        }

        lua_pop(L, 2);

        return 0;
    }

    return n;
}


static int
lua_json_array_append(lua_State *L, int len, lua_json_builder_t *json)
{
    int        i;
    ngx_chb_t  *b;

    b = &json->chb;

    ngx_chb_add_char(b, '[');

    for (i = 1; i <= len; i++) {
        if (i > 1) {
            ngx_chb_add_char(b, ',');
        }

        lua_rawgeti(L, -1, i);

        if (lua_json_value_append(L, json)) {
            return -1;
        }

        lua_pop(L, 1);
    }

    ngx_chb_add_char(b, ']');

    return 0;
}


static int
lua_json_object_append(lua_State *L, lua_json_builder_t *json)
{
    int        has_content, type;
    ngx_chb_t  *b;

    b = &json->chb;

    ngx_chb_add_char(b, '{');

    has_content = 0;

    lua_pushnil(L);

    while (lua_next(L, -2) != 0) {
        if (has_content) {
            ngx_chb_add_char(b, ',');
        }

        type = lua_type(L, -2);

        if (type == LUA_TNUMBER) {
            if (lua_json_number_append(L, json, 1)) {
                return -1;
            }

        } else if (type == LUA_TSTRING) {
            if (lua_json_string_append(L, json, 1)) {
                return -1;
            }

        } else {
            return lua_json_build_error(json, "object key must be "
                                              "a number or string");
        }

        if (lua_json_value_append(L, json)) {
            return -1;
        }

        has_content = 1;

        lua_pop(L, 1);
    }

    ngx_chb_add_char(b, '}');

    return 0;
}


static ngx_inline int
lua_json_build_error(lua_json_builder_t *json, const char *err)
{
    json->error = err;
    return -1;
}


enum {
    LUA_TOKEN_NULL,
    LUA_TOKEN_BOOLEAN,
    LUA_TOKEN_NUMBER,
    LUA_TOKEN_STRING,
    LUA_TOKEN_ESCAPE_STRING,
    LUA_TOKEN_EOF,
};

typedef struct {
    int                 val;
    union {
        int             boolean;
        double          number;
        ngx_str_t       string;
    } u;
} lua_json_token_t;

struct lua_json_parser_s {
    lua_json_token_t    token;
    u_char              *buf_start;
    u_char              *buf_ptr;
    u_char              *buf_end;
    ngx_str_t           error;
};


static int
lua_json_decode(lua_State *L)
{
    ngx_str_t          str;
    lua_json_parser_t  json;

    str.data = (u_char *) luaL_checklstring(L, 1, &str.len);

    ngx_memzero(&json, sizeof(lua_json_parser_t));

    json.buf_start = str.data;
    json.buf_ptr = str.data;
    json.buf_end = str.data + str.len;

    if (lua_json_next_token(L, &json)) {
        goto fail;
    }

    if (lua_json_value_parse(L, &json)) {
        goto fail;
    }

    if (json.token.val != LUA_TOKEN_EOF) {
        lua_json_parse_error(L, &json, "unexpected end of input");
        goto fail;
    }

    return 1;

fail:

    lua_pushnil(L);
    lua_pushlstring(L, (const char *) json.error.data, json.error.len);

    return 2;
}


static int
lua_json_next_token(lua_State *L, lua_json_parser_t *json)
{
    u_char  c, *p;

    p = json->buf_ptr;

    while (1) {
        c = *p;

        switch (c) {

        case '\0':
            if (p == json->buf_end) {
                json->token.val = LUA_TOKEN_EOF;
                return 0;
            }

            goto token;

        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9': case '-':
            if (lua_json_next_number(L, json, p, &p)) {
                return -1;
            }

            break;

        case '"':
            if (lua_json_next_string(L, json, p + 1, &p)) {
                return -1;
            }

            break;

        case 't':
            if (ngx_strncmp(p, "true", 4) == 0) {
                p += 4;
                json->token.val = LUA_TOKEN_BOOLEAN;
                json->token.u.boolean = 1;
                break;
            }

            goto token;

        case 'f':
            if (ngx_strncmp(p, "false", 5) == 0) {
                p += 5;
                json->token.val = LUA_TOKEN_BOOLEAN;
                json->token.u.boolean = 1;
                break;
            }

            goto token;

        case 'n':
            if (ngx_strncmp(p, "null", 4) == 0) {
                p += 4;
                json->token.val = LUA_TOKEN_NULL;
                break;
            }

            goto token;

        case ' ':
        case '\t':
        case '\n':
        case '\r':
            p++;
            continue;

        token:
        default:
            p++;
            json->token.val = c;
            break;
        }

        json->buf_ptr = p;
        break;
    }

    return 0;
}


static int
lua_json_expect(lua_State *L, lua_json_parser_t *json, char c)
{
    if (json->token.val != c) {
        return lua_json_parse_error(L, json, "expecting '%c'", c);
    }

    return lua_json_next_token(L, json);
}


static int
lua_json_value_parse(lua_State *L, lua_json_parser_t *json)
{
    lua_json_token_t  *token;

    token = &json->token;

    switch (token->val) {

    case LUA_TOKEN_NULL:
        lua_pushnil(L);
        break;

    case LUA_TOKEN_BOOLEAN:
        lua_pushboolean(L, token->u.boolean);
        break;

    case LUA_TOKEN_NUMBER:
        lua_pushnumber(L, token->u.number);
        break;

    case LUA_TOKEN_STRING:
    case LUA_TOKEN_ESCAPE_STRING:
        if (lua_json_string_parse(L, json)) {
            return -1;
        }
        break;

    case '[':
        return lua_json_array_parse(L, json);

    case '{':
        return lua_json_object_parse(L, json);

    default:
        return lua_json_parse_error(L, json, "unexpected token '%c'",
                                    token->val);
    }

    return lua_json_next_token(L, json);
}


static int
lua_json_next_number(lua_State *L, lua_json_parser_t *json, u_char *p,
    u_char **pp)
{
    int     len;
    char    *endptr, buf[32];
    double  num;

    ngx_memcpy(buf, p, 31);
    buf[31] = '\0';

    num = strtod(buf, &endptr);
    len = endptr - buf;

    if (len == 0) {
        return lua_json_parse_error(L, json, "invalid number");
    }

    json->token.val = LUA_TOKEN_NUMBER;
    json->token.u.number = num;

    *pp += len;

    return 0;
}


static int
lua_json_next_string(lua_State *L, lua_json_parser_t *json, u_char *p,
    u_char **pp)
{
    int               escape;
    u_char            c;
    lua_json_token_t  *token;

    escape = 0;

    token = &json->token;
    token->u.string.data = p;

    while (*p != '"') {
        c = *p++;

        if (c == '\0') {
            json->buf_ptr = p - 1;
            return lua_json_parse_error(L, json, "unexpected end of string");
        }

        if (c == '\\') {
            escape = 1;
        }
    }

    token->u.string.len = p - token->u.string.data;
    token->val = escape ? LUA_TOKEN_ESCAPE_STRING : LUA_TOKEN_STRING;

    *pp = p + 1;

    return 0;
}


static ngx_inline int
lua_from_hex(u_char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';

    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;

    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;

    } else {
        return -1;
    }
}


static int
lua_get_hex(u_char *str, int n)
{
    int  i, c, h;

    c = 0;

    for (i = 0; i < n; i++) {
        h = lua_from_hex(str[i]);
        if (h < 0) {
            return -1;
        }

        c = (c << 4) | h;
    }

    return c;
}


static int
lua_utf8_encode(u_char *buf, int c)
{
    /* 0xxxxxxx */
    if (c <= 0x7F) {
        buf[0] = c;

        return 1;
    }

    /* 110xxxxx 10xxxxxx */
    if (c <= 0x7FF) {
        buf[0] = (c >> 6) | 0xC0;
        buf[1] = (c & 0x3F) | 0x80;

        return 2;
    }

    /* 1110xxxx 10xxxxxx 10xxxxxx */
    if (c <= 0xFFFF) {
        buf[0] = (c >> 12) | 0xE0;
        buf[1] = ((c >> 6) & 0x3F) | 0x80;
        buf[2] = (c & 0x3F) | 0x80;

        return 3;
    }

    /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    if (c <= 0x1FFFFF) {
        buf[0] = (c >> 18) | 0xF0;
        buf[1] = ((c >> 12) & 0x3F) | 0x80;
        buf[2] = ((c >> 6) & 0x3F) | 0x80;
        buf[3] = (c & 0x3F) | 0x80;

        return 4;
    }

    return 0;
}


static u_char *
lua_unicode_escape(u_char *p, u_char **dst)
{
    size_t     len;
    u_char     utf8[4];
    ngx_int_t  c, c1;

    c = lua_get_hex(p, 4);
    if (c < 0) {
        return NULL;
    }

    p += 4;

    if ((c & 0xF800) == 0xD800) {

        /* check if the first surrogate is not high */
        if (c & 0x400) {
            return NULL;
        }

        if (p[0] != '\\' || p[1] != 'u') {
            return NULL;
        }

        c1 = lua_get_hex(p + 2, 4);
        if (c1 < 0) {
            return NULL;
        }

        p += 6;

        /* check if the second code is not a low surrogate */
        if ((c1 & 0xFC00) != 0xDC00) {
            return NULL;
        }

        c = (((c & 0x3ff) << 10) | (c1 & 0x3ff)) + 0x10000;
    }

    len = lua_utf8_encode(utf8, c);
    if (len == 0) {
        return NULL;
    }

    *dst = ngx_cpymem(*dst, utf8, len);

    return p;
}


static int
lua_json_string_parse(lua_State *L, lua_json_parser_t *json)
{
    u_char            c, *p, *dst, *last;
    ngx_str_t         *str;
    lua_json_token_t  *token;

    token = &json->token;
    str = &token->u.string;

    if (token->val == LUA_TOKEN_STRING) {
        lua_pushlstring(L, (const char *) str->data, str->len);
        return 0;
    }

    /* LUA_TOKEN_ESCAPE_STRING */

    dst = malloc(str->len);
    if (dst == NULL) {
        return luaL_error(L, "json string parse failed");
    }

    last = dst;

    p = str->data;

    while (*p != '"') {
        c = *p++;

        if (c != '\\') {
            *last++ = c;
            continue;
        }

        c = *p++;

        switch (c) {

        case '"':
        case '\\':
        case '/':
            *last++ = c;
            continue;

        case 'n':
            *last++ = '\n';
            continue;

        case 'r':
            *last++ = '\r';
            continue;

        case 't':
            *last++ = '\t';
            continue;

        case 'b':
            *last++ = '\b';
            continue;

        case 'f':
            *last++ = '\f';
            continue;

        case 'u':
            p = lua_unicode_escape(p, &last);
            if (p == NULL) {
                lua_json_parse_error(L, json, "invalid unicode escape code");
                goto fail;
            }

            continue;

        default:
            lua_json_parse_error(L, json, "invalid escape code");
            goto fail;
        }
    }

    lua_pushlstring(L, (const char *) dst, last - dst);

    free(dst);

    return 0;

fail:

    free(dst);

    return -1;
}


static int
lua_json_array_parse(lua_State *L, lua_json_parser_t *json)
{
    int  i;

    lua_newtable(L);

    if (lua_json_next_token(L, json)) {
        return -1;
    }

    if (json->token.val == ']') {
        return 0;
    }

    for (i = 1; ; i++) {
        if (lua_json_value_parse(L, json)) {
            return -1;
        }

        lua_rawseti(L, -2, i);

        if (json->token.val != ',') {
            break;
        }

        if (lua_json_next_token(L, json)) {
            return -1;
        }
    }

    return lua_json_expect(L, json, ']');
}


static int
lua_json_object_parse(lua_State *L, lua_json_parser_t *json)
{
    lua_newtable(L);

    if (lua_json_next_token(L, json)) {
        return -1;
    }

    if (json->token.val == '}') {
        return 0;
    }

    for (;;) {
        if (json->token.val != LUA_TOKEN_STRING) {
            return lua_json_parse_error(L, json, "invalid property name");
        }

        lua_pushlstring(L, (const char *) json->token.u.string.data,
                        json->token.u.string.len);

        if (lua_json_next_token(L, json)) {
            return -1;
        }

        if (lua_json_expect(L, json, ':')) {
            return -1;
        }

        if (lua_json_value_parse(L, json)) {
            return -1;
        }

        lua_rawset(L, -3);

        if (json->token.val != ',') {
            break;
        }

        if (lua_json_next_token(L, json)) {
            return -1;
        }
    }

    return lua_json_expect(L, json, '}');
}


static ngx_inline int
lua_json_parse_error(lua_State *L, lua_json_parser_t *json,
    const char *fmt, ...)
{
    va_list    args;
    ngx_lua_t  *lua;
    u_char     *p, *end, errstr[NGX_MAX_ERROR_STR];

    lua = ngx_lua_ext_get(L);
    end = errstr + NGX_MAX_ERROR_STR;

    va_start(args, fmt);
    p = ngx_vslprintf(errstr, end, fmt, args);
    va_end(args);

    p = ngx_slprintf(p, end, " in json at position %d",
                     json->buf_ptr - json->buf_start);

    json->error.len = p - errstr;
    json->error.data = ngx_palloc(lua->pool, json->error.len);
    if (json->error.data == NULL) {
        return -1;
    }

    ngx_memcpy(json->error.data, errstr, json->error.len);

    return -1;
}


static const struct luaL_Reg  lua_json_methods[] = {
    {"json_encode", lua_json_encode},
    {"json_decode", lua_json_decode},
    {NULL, NULL},
};


void
ngx_lua_json_register(lua_State *L)
{
    luaL_setfuncs(L, lua_json_methods, 0);
}
