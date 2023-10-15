# nginx-http-lua-module
A module to play with lua 5.4.

Compatibility
=============

- NGINX 1.25.0+
- Lua 5.4.0+

Build
=====

Configuring nginx with the module.

    $ ./configure --add-module=/path/to/nginx-http-lua-module
    
Directives
==========

- ``lua_script``
- ``lua_timer``
- ``lua_shared_dict_zone``

nginx object
====
- ``ngx.shared``
- ``ngx.json_encode(val)``
- ``ngx.json_decode(str)``
- ``ngx.base64_encode(str)``
- ``ngx.base64_decode(str)``
- ``ngx.cidr_parse(addr)``

request object
====
- ``r.uri``
- ``r.method``
- ``r.client_ip``
- ``r.body``
- ``r.args``
- ``r.vars``
- ``r.headers``
- ``r.resp``
- ``r.echo(text)``
- ``r.exit(status)``
- ``r.match_cidr(cidr)``

headers object
====
- ``headers.get(name)``
- ``headers.set(name, value)``

response object
====
- ``resp.headers``


Example
=======

nginx.conf
```
events {}

http {
    lua_shared_dict_zone  zone=config:1M;

    server {
        listen 8000;

        location /hello {
            lua_script  "r.echo('hello');r.exit(200)";
        }

        location /dump {
            lua_script  "require('http').dump(r)";
        }

        location /timer {
            lua_timer  "require('http').timer(r)";
        }
    }
}
```

http.lua
```
local _M = {};

function _M.dump(r)
    local headers = r.headers;
    local val = {
        uri = r.uri,
        method = r.method,
        client_ip = r.client_ip,
        foo = r.args['foo'],
        baz = headers:get('baz'),
    };

    local text = ngx.json_encode(val);

    local headers = r.resp.headers;
    headers:set("Content-type", "application/json");

    r.echo(text);
end

function _M.timer(r)
    local config = ngx.shared.config;

    config:set('version', '0.1.0');
    local version = config:get('version');

    print(version);
end

return _M;
```

Community
=========
Author: Zhidao HONG <hongzhidao@gmail.com>
