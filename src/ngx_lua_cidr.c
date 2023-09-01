
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_lua_core.h>


static int
lua_cidr_parse(lua_State *L)
{
    ngx_int_t   ret;
    ngx_str_t   addr;
    ngx_cidr_t  cidr, *pcidr;

    addr.data = (u_char *) luaL_checklstring(L, 1, &addr.len);

    ret = ngx_ptocidr(&addr, &cidr);

    if (ret == NGX_ERROR) {
        lua_pushnil(L);
        lua_pushstring(L, "invalid cidr");
        return 2;
    }

    pcidr = lua_newuserdata(L, sizeof(ngx_cidr_t));
    if (pcidr == NULL) {
        return luaL_error(L, "cidr parse failed");
    }

    *pcidr = cidr;

    return 1;
}


static inline int
lua_cidr_match_inet(ngx_cidr_t *cidr, in_addr_t addr)
{
    return (addr & cidr->u.in.mask) == cidr->u.in.addr;
}


#if (NGX_HAVE_INET6)

static inline int
lua_cidr_match_inet6(ngx_cidr_t *cidr, u_char *p)
{
    ngx_uint_t  i;

    for (i = 0; i < 16; i++) {
        if ((p[i] & cidr->u.in6.mask.s6_addr[i])
            != cidr->u.in6.addr.s6_addr[i])
        {
            return 0;
        }
    }

    return 1;
}

#endif


int
ngx_lua_cidr_match(ngx_cidr_t *cidr, struct sockaddr *sockaddr)
{
    in_addr_t            addr;
    struct sockaddr_in   *sin;
#if (NGX_HAVE_INET6)
    u_char               *p;
    struct sockaddr_in6  *sin6;
#endif

    if (cidr->family != sockaddr->sa_family) {
        return 0;
    }

    switch (sockaddr->sa_family) {

    case AF_INET:
        sin = (struct sockaddr_in *) sockaddr;
        addr = sin->sin_addr.s_addr;

        return lua_cidr_match_inet(cidr, sin->sin_addr.s_addr);

    #if (NGX_HAVE_INET6)

    case AF_INET6:
        sin6 = (struct sockaddr_in6 *) sockaddr;
        p = sin6->sin6_addr.s6_addr;

        if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
            addr = p[12] << 24;
            addr += p[13] << 16;
            addr += p[14] << 8;
            addr += p[15];

            return lua_cidr_match_inet(cidr, htonl(addr));

        } else {
            return lua_cidr_match_inet6(cidr, p);
        }

#endif

    default:
        return 0;
    }
}


static const struct luaL_Reg  lua_cidr_methods[] = {
    {"cidr_parse", lua_cidr_parse},
    {NULL, NULL},
};


void
ngx_lua_cidr_register(lua_State *L)
{
    luaL_setfuncs(L, lua_cidr_methods, 0);
}
