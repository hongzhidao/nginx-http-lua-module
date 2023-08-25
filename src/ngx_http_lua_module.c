
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_lua_core.h>
#include <ngx_lua_http.h>

typedef struct {
    int             lua_ref;
} ngx_http_lua_loc_conf_t;

static ngx_int_t ngx_http_lua_dict_init_zone(ngx_shm_zone_t *shm_zone,
    void *data);
static ngx_int_t ngx_http_lua_init(ngx_conf_t *cf);
static void *ngx_http_lua_create_main_conf(ngx_conf_t *cf);
static void *ngx_http_lua_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_lua_merge_loc_conf(ngx_conf_t *cf, void *parent,
    void *child);
static char *ngx_http_lua_script(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_lua_dict_zone(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);


static ngx_command_t  ngx_http_lua_commands[] = {

    { ngx_string("lua_script"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_lua_script,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("lua_shared_dict_zone"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_1MORE,
      ngx_http_lua_dict_zone,
      0,
      0,
      NULL },

    ngx_null_command
};


static ngx_http_module_t  ngx_http_lua_module_ctx = {
    NULL,                          /* preconfiguration */
    ngx_http_lua_init,             /* postconfiguration */

    ngx_http_lua_create_main_conf, /* create main configuration */
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


static void
ngx_http_lua_body_handler(ngx_http_request_t *r)
{
    ngx_int_t                 ret;
    ngx_lua_t                 *lua;
    ngx_http_lua_loc_conf_t   *llcf;
    ngx_http_lua_main_conf_t  *lmcf;
    ngx_http_complex_value_t  cv;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http lua body handler");

    lmcf = ngx_http_get_module_main_conf(r, ngx_http_lua_module);
    llcf = ngx_http_get_module_loc_conf(r, ngx_http_lua_module);

    lua = ngx_lua_clone(r->pool, lmcf->lua);
    if (lua == NULL) {
        goto fail;
    }

    lua->data = r;
    lua->log = r->connection->log;

    ngx_http_set_ctx(r, lua, ngx_http_lua_module);

    lua_rawgeti(lua->state, LUA_REGISTRYINDEX, llcf->lua_ref);
    lua_rawgeti(lua->state, LUA_REGISTRYINDEX, lmcf->request_ref);

    ret = ngx_lua_call(lua, 1);
    if (ret != NGX_OK) {
        goto fail;
    }

    if (lua->status > 0 || lua->buf != NULL) {
        ngx_memzero(&cv, sizeof(ngx_http_complex_value_t));

        if (lua->status == 0) {
            lua->status = NGX_HTTP_OK;
        }

        if (lua->buf != NULL) {
            cv.value.data = lua->buf->pos;
            cv.value.len = ngx_buf_size(lua->buf);
        }

        ret = ngx_http_send_response(r, lua->status, NULL, &cv);
        if (ret == NGX_ERROR) {
            goto fail;
        }

        ngx_http_finalize_request(r, ret);
        return;
    }

    r->write_event_handler = ngx_http_core_run_phases;
    ngx_http_core_run_phases(r);

    return;

fail:

    ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
}


static ngx_int_t
ngx_http_lua_handler(ngx_http_request_t *r)
{
    ngx_int_t                ret;
    ngx_lua_t                *lua;
    ngx_http_lua_loc_conf_t  *llcf;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http lua handler");

    llcf = ngx_http_get_module_loc_conf(r, ngx_http_lua_module);

    if (llcf->lua_ref == 0) {
        return NGX_DECLINED;
    }

    lua = ngx_http_get_module_ctx(r, ngx_http_lua_module);
    if (lua != NULL) {
        return NGX_DECLINED;
    }

    ret = ngx_http_read_client_request_body(r, ngx_http_lua_body_handler);
    if (ret >= NGX_HTTP_SPECIAL_RESPONSE) {
        return ret;
    }

    ngx_http_finalize_request(r, NGX_DONE);

    return NGX_DONE;
}


static void *
ngx_http_lua_create_main_conf(ngx_conf_t *cf)
{
    ngx_lua_t                 *lua;
    ngx_http_lua_main_conf_t  *lmcf;

    lmcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_lua_main_conf_t));
    if (lmcf == NULL) {
        return NULL;
    }

    lua = ngx_lua_create(cf->pool);
    if (lua == NULL) {
        return NULL;
    }

    ngx_lua_core_register(lua->state);

    lmcf->lua = lua;
    lmcf->request_ref = ngx_lua_http_object_ref(lua->state);

    lmcf->dicts = ngx_array_create(cf->pool, 4, sizeof(ngx_lua_dict_t));
    if (lmcf->dicts == NULL) {
        return NULL;
    }

    return lmcf;
}


static void *
ngx_http_lua_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_lua_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_lua_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->lua_ref = 0;
     */

    return conf;
}


static char *
ngx_http_lua_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_lua_loc_conf_t  *prev = parent;
    ngx_http_lua_loc_conf_t  *conf = child;

    if (conf->lua_ref == 0) {
        conf->lua_ref = prev->lua_ref;
    }

    return NGX_CONF_OK;
}


static const char *
ngx_http_lua_reader(lua_State *L, void *ud, size_t *size)
{
    u_char     *buffer;
    ngx_str_t  *script;

    static const ngx_str_t  prefix = ngx_string("local r = ...;");

    script = ud;

    if (script->len == 0) {
        return NULL;
    }

    buffer = lua_newuserdata(L, prefix.len + script->len);
    if (buffer == NULL) {
        return NULL;
    }

    ngx_memcpy(buffer, prefix.data, prefix.len);
    ngx_memcpy(buffer + prefix.len, script->data, script->len);

    *size = prefix.len + script->len;
    script->len = 0;

    return (const char *) buffer;
}


static char *
ngx_http_lua_script(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_lua_loc_conf_t *llcf = conf;

    int                       ret;
    ngx_str_t                 *value, script;
    ngx_lua_t                 *lua;
    ngx_http_lua_main_conf_t  *lmcf;

    if (llcf->lua_ref != 0) {
        return "is duplicate";
    }

    lmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_lua_module);
    lua = lmcf->lua;

    value = cf->args->elts;
    script = value[1];

    ret = lua_load(lua->state, ngx_http_lua_reader, &script, NULL, NULL);

    if (ret != LUA_OK) {
        ngx_log_error(NGX_LOG_ERR, cf->log, 0, "lua_load() failed");
        return NGX_CONF_ERROR;
    }

    llcf->lua_ref = luaL_ref(lua->state, LUA_REGISTRYINDEX);

    return NGX_CONF_OK;
}


static char *
ngx_http_lua_dict_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_lua_main_conf_t  *lmcf = conf;

    u_char          *p;
    ssize_t         size;
    ngx_str_t       *value, name, s;
    ngx_uint_t      i;
    ngx_lua_dict_t  *dict;
    ngx_shm_zone_t  *shm_zone;

    size = 0;
    name.len = 0;

    value = cf->args->elts;

    for (i = 1; i < cf->args->nelts; i++) {

        if (ngx_strncmp(value[i].data, "zone=", 5) == 0) {
            name.data = value[i].data + 5;

            p = (u_char *) ngx_strchr(name.data, ':');

            if (p == NULL) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "invalid zone size \"%V\"", &value[i]);
                return NGX_CONF_ERROR;
            }

            name.len = p - name.data;

            if (name.len == 0) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "invalid zone name \"%V\"", &value[i]);
                return NGX_CONF_ERROR;
            }

            s.data = p + 1;
            s.len = value[i].data + value[i].len - s.data;

            size = ngx_parse_size(&s);

            if (size == NGX_ERROR) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "invalid zone size \"%V\"", &value[i]);
                return NGX_CONF_ERROR;
            }

            if (size < (ssize_t) (8 * ngx_pagesize)) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "zone \"%V\" is too small", &value[i]);
                return NGX_CONF_ERROR;
            }

            continue;
        }
    }

    if (name.len == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"%V\" must have \"zone\" parameter", &cmd->name);
        return NGX_CONF_ERROR;
    }

    shm_zone = ngx_shared_memory_add(cf, &name, size, &ngx_http_lua_module);
    if (shm_zone == NULL) {
        return NGX_CONF_ERROR;
    }

    if (shm_zone->data) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "duplicate zone \"%V\"",
                           &name);
        return NGX_CONF_ERROR;
    }

    dict = ngx_array_push(lmcf->dicts);
    if (dict == NULL) {
        return NGX_CONF_ERROR;
    }

    dict->shm_zone = shm_zone;

    shm_zone->init = ngx_http_lua_dict_init_zone;
    shm_zone->data = dict;

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_lua_dict_init_zone(ngx_shm_zone_t *shm_zone, void *data)
{
    ngx_lua_dict_t  *prev = data;

    size_t          len;
    ngx_lua_dict_t  *dict;

    dict = shm_zone->data;

    if (prev) {
        dict->sh = prev->sh;
        dict->shpool = prev->shpool;

        return NGX_OK;
    }

    dict->shpool = (ngx_slab_pool_t *) shm_zone->shm.addr;

    if (shm_zone->shm.exists) {
        dict->sh = dict->shpool->data;
        return NGX_OK;
    }

    dict->sh = ngx_slab_calloc(dict->shpool, sizeof(ngx_lua_dict_sh_t));
    if (dict->sh == NULL) {
        return NGX_ERROR;
    }

    dict->shpool->data = dict->sh;

    ngx_rbtree_init(&dict->sh->rbtree, &dict->sh->sentinel,
                    ngx_str_rbtree_insert_value);

    len = sizeof(" in lua shared dict zone \"\"") + shm_zone->shm.name.len;

    dict->shpool->log_ctx = ngx_slab_alloc(dict->shpool, len);
    if (dict->shpool->log_ctx == NULL) {
        return NGX_ERROR;
    }

    ngx_sprintf(dict->shpool->log_ctx, " in lua shared zone \"%V\"%Z",
                &shm_zone->shm.name);

    return NGX_OK;
}


static ngx_int_t
ngx_http_lua_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_PRECONTENT_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_lua_handler;

    return NGX_OK;
}
