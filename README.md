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
- ``ngx.json_encode(val)``
- ``ngx.json_decode(str)``
- ``ngx.shared``

request object
====
- ``r.uri``
- ``r.method``
- ``r.client_ip``
- ``r.body``
- ``r.args``
- ``r.headers``
- ``r.vars``
- ``r.echo(text)``
- ``r.exit(status)``
- ``r.set_header(name, value)``


Example
=======

nginx.conf
```
events {}

http {
    lua_shared_dict_zone  zone=config:1M;

    server {
        listen  8081;
        listen  8082;
        return  200 $server_port;
    }

    server {
        listen 8000;

        location /hello {
            lua_script  "r.echo('hello');r.exit(200)";
        }

        location /foo {
            lua_script  "require('http').foo(r)";
        }

        location /proxy {
            set  $backend '127.0.0.1:8081';
            lua_script  "r.vars.backend = '127.0.0.1:8082';
            proxy_pass  http://$backend;
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

function _M.foo(r)
    local config = ngx.shared.config;

    local val = {
        uri = r.uri,
        method = r.method,
        client_ip = r.client_ip,
        foo = r.args['foo'],
        baz = r.headers['baz'],
    };

    local text = ngx.json_encode(val);

    config:set('data', text);

    r.set_header("Content-type", "application/json");
    r.echo(text);
end

function _M.timer(r)
    local config = ngx.shared.config;
    local data = config:get('data');
    print(data);
end

return _M;
```

Community
=========
Author: Zhidao HONG <hongzhidao@gmail.com>
