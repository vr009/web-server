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
