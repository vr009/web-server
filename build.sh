git submodule init &&
git submodule add https://github.com/mksdev/libev-cmake &&
cd libev-cmake &&
git submodule add https://github.com/enki/libev &&
cd .. &&
mkdir build && cd build && cmake .. && make
