# web_server

## Motivation

This an educational project for getting experience on creating such applications.

## Architcture

This is epoll-based (kqueue-based for OSX) tcp-server that works asynchroniously.

## How to run

```
$ docker build -t web_server
$ docker run -p 80:80 web_server
```

## Server configuration

Web_server supports two types of configuration files plain text and yaml format. 
It is possible to set count of used CPU's, lua script file and root of static files.
Examples.
Plain text config:
```
cpu_limit 4       # maximum CPU count to use
document_root /var/www/html
script hello.lua
```

YAML foramt config:
```yaml
cpu_limit: 4
document_root: /var/www/html
script: hello.lua
```
