
/*
 * Copyright (C) Zhidao HONG
 */

#ifndef NGX_CHB_H
#define NGX_CHB_H

#include <ngx_config.h>
#include <ngx_core.h>

/*
 * This chb is based on the njs open-source project's code
 * with some modifications.
 */

typedef struct ngx_chb_node_s  ngx_chb_node_t;

struct ngx_chb_node_s {
    ngx_chb_node_t      *next;
    u_char              *start;
    u_char              *pos;
    u_char              *end;
};

typedef struct {
    ngx_pool_t          *pool;
    ngx_chb_node_t      *nodes;
    ngx_chb_node_t      *last;
    uint8_t             error;
} ngx_chb_t;

static inline void
ngx_chb_init(ngx_chb_t *b, ngx_pool_t *pool)
{
    b->pool = pool;
    b->nodes = NULL;
    b->last = NULL;
    b->error = 0;
}

u_char *ngx_chb_reserve(ngx_chb_t *b, size_t size);
void ngx_chb_add_string(ngx_chb_t *b, u_char *data, size_t len);
void ngx_chb_add_char(ngx_chb_t *b, u_char c);
ngx_int_t ngx_chb_join(ngx_chb_t *b, ngx_str_t *str);

#define ngx_chb_add(b, str)                                                   \
    ngx_chb_add_string(b, (u_char *) str, sizeof(str) - 1)

static inline void
ngx_chb_advance(ngx_chb_t *b, u_char *p)
{
    b->last->pos = p;
}

#endif /* NGX_CHB_H */
