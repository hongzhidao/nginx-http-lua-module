
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_lua.h>
#include <ngx_http.h>

typedef struct {
    ngx_lua_t       *lua;
} ngx_http_lua_conf_t;

static ngx_int_t ngx_http_lua_init(ngx_conf_t *cf);
static void *ngx_http_lua_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_lua_merge_loc_conf(ngx_conf_t *cf, void *parent,
    void *child);
static char *ngx_http_lua_script(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);


static ngx_command_t  ngx_http_lua_commands[] = {

    { ngx_string("lua_script"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_lua_script,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    ngx_null_command
};


static ngx_http_module_t  ngx_http_lua_module_ctx = {
    NULL,                          /* preconfiguration */
    ngx_http_lua_init,             /* postconfiguration */

    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */

    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */

    ngx_http_lua_create_loc_conf,  /* create location configuration */
    ngx_http_lua_merge_loc_conf    /* merge location configuration */
};


ngx_module_t  ngx_http_lua_module = {
    NGX_MODULE_V1,
    &ngx_http_lua_module_ctx,      /* module context */
    ngx_http_lua_commands,         /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_lua_handler(ngx_http_request_t *r)
{
    ngx_int_t            rc;
    ngx_lua_t            *lua;
    ngx_http_lua_conf_t  *lcf;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http lua handler");

    lcf = ngx_http_get_module_loc_conf(r, ngx_http_lua_module);

    if (lcf->lua == NULL) {
        return NGX_DECLINED;
    }

    lua = ngx_lua_clone(r->pool, lcf->lua);
    if (lua == NULL) {
        return NGX_ERROR;
    }

    lua->data = r;

    rc = ngx_lua_call(lua, r->connection->log);
    if (rc != NGX_OK) {
        return NGX_ERROR;
    }

    if (lua->status > 0) {
        return lua->status;
    }

    return NGX_OK;
}


static void *
ngx_http_lua_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_lua_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_lua_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->lua = NULL;
     */

    return conf;
}


static char *
ngx_http_lua_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_lua_conf_t  *prev = parent;
    ngx_http_lua_conf_t  *conf = child;

    if (conf->lua == NULL) {
        conf->lua = prev->lua;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_lua_script(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_lua_conf_t *lcf = conf;

    int        ret;
    ngx_str_t  *value;
    ngx_lua_t  *lua;

    if (lcf->lua != NULL) {
        return "is duplicate";
    }

    value = cf->args->elts;

    lua = ngx_lua_create(cf, &value[1]);
    if (lua == NULL) {
        return NGX_CONF_ERROR;
    }

    lcf->lua = lua;

    ngx_lua_http_register(lua->state);

    ret = luaL_loadstring(lua->state, (const char *) value[1].data);

    if (ret != LUA_OK) {
        ngx_log_error(NGX_LOG_ERR, cf->log, 0, "luaL_loadfile() failed");
        return NGX_CONF_ERROR;
    }

    lua->ref = luaL_ref(lua->state, LUA_REGISTRYINDEX);

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_lua_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_lua_handler;

    return NGX_OK;
}
