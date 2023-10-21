
/*
 * Copyright (C) Zhidao HONG
 */

#include <ngx_chb.h>

#define NGX_CHB_MIN_SIZE  1024


u_char *
ngx_chb_reserve(ngx_chb_t *b, size_t size)
{
    ngx_chb_node_t  *node;

    node = b->last;

    if (node != NULL && ngx_chb_node_free(node) >= size) {
        return node->pos;
    }

    if (size < NGX_CHB_MIN_SIZE) {
        size = NGX_CHB_MIN_SIZE;
    }

    node = ngx_palloc(b->pool, sizeof(ngx_chb_node_t) + size);
    if (node == NULL) {
        b->error = 1;
        return NULL;
    }

    node->next = NULL;
    node->start = (u_char *) node + sizeof(ngx_chb_node_t);
    node->pos = node->start;
    node->end = node->pos + size;

    if (b->last != NULL) {
        b->last->next = node;

    } else {
        b->nodes = node;
    }

    b->last = node;

    return node->start;
}


void
ngx_chb_add_string(ngx_chb_t *b, u_char *data, size_t len)
{
    u_char  *p;

    if (!b->error) {
        p = ngx_chb_reserve(b, len);
        if (p == NULL) {
            return;
        }

        p = ngx_cpymem(p, data, len);

        ngx_chb_advance(b, p);
    }
}


void
ngx_chb_add_char(ngx_chb_t *b, u_char c)
{
    u_char  *p;

    if (!b->error) {
        p = ngx_chb_reserve(b, 1);
        if (p == NULL) {
            return;
        }

        *b->last->pos++ = c;
    }
}


ngx_int_t
ngx_chb_join(ngx_chb_t *b, ngx_str_t *str)
{
    size_t  size;
    u_char  *start;

    if (b->error) {
        return NGX_ERROR;
    }

    size = ngx_chb_size(b);

    if (size == 0) {
        str->len = 0;
        str->data = NULL;
        return NGX_OK;
    }

    if (size >= UINT32_MAX) {
        return NGX_ERROR;
    }

    if (b->last == b->nodes) {
        str->len = size;
        str->data = b->last->start;
        return NGX_OK;
    }

    start = ngx_palloc(b->pool, size);
    if (start == NULL) {
        return NGX_ERROR;
    }

    str->len = size;
    str->data = start;

    ngx_chb_join_write(b, start);

    return NGX_OK;
}


void
ngx_chb_join_write(ngx_chb_t *b, u_char *dst)
{
    size_t          size;
    ngx_chb_node_t  *node;

    node = b->nodes;

    while (node != NULL) {
        size = ngx_chb_node_used(node);
        dst = ngx_cpymem(dst, node->start, size);
        node = node->next;
    }
}


static void
ngx_chb_vsprintf(ngx_chb_t *chain, size_t size, const char *fmt, va_list args)
{
    u_char  *start, *end;

    start = ngx_chb_reserve(chain, size);
    if (start == NULL) {
        return;
    }

    end = ngx_vslprintf(start, start + size, fmt, args);

    ngx_chb_advance(chain, end);
}


void
ngx_chb_sprintf(ngx_chb_t *chain, size_t size, const char* fmt, ...)
{
    va_list  args;

    va_start(args, fmt);

    ngx_chb_vsprintf(chain, size, fmt, args);

    va_end(args);
}
