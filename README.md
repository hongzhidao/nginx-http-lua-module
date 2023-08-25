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
- ``r.body``
- ``r.echo(text)``
- ``r.exit(status)``


Example
=======

nginx.conf
```
events {}

http {
    lua_shared_dict_zone  zone=config:1M;

    server {
        listen 8000;

        location / {
            lua_script  "r.exit(206)";
        }

        location /hello {
            lua_script  "r.echo('hello ' .. r.uri)";
        }

        location /test {
            lua_script  "require('http').test(r)";
        }

        location /timer {
            lua_timer  "print('timer test')";
        }
    }
}
```

http.lua
```
local _M = {};

local str = [[
{
    "num": [1, 2, 3]
}
]];

function _M.test(r)
    local config = ngx.shared.config;

    config:set('data', str);
    local data = config:get('data');

    local val = ngx.json_decode(data);
    local text = ngx.json_encode(val);

    r.echo(text);
    r.exit(200);
end

return _M;
```

Community
=========
Author: Zhidao HONG <hongzhidao@gmail.com>
