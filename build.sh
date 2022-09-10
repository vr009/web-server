#!/usr/bin/env bash

apt-get update && apt-get install autoconf git gcc build-essential -y &&
git submodule init &&
git submodule update --recursive &&
git clone https://github.com/enki/libev &&
cd libev && ./configure && make && make install &&
cd .. && rm -rf build && mkdir build && cd build && cmake .. && make
