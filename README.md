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

- ``lua_script`` (http|server|location)

nginx object
====
- ``ngx.json_encode(val)``
- ``ngx.json_decode(str)``

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
    server {
        listen 8000;

        location / {
            lua_script  "r.exit(206)";
        }

        location /hello {
            lua_script  "r.echo('hello ' .. r.uri)";
        }
    }
}
```

Community
=========
Author: Zhidao HONG <hongzhidao@gmail.com>
