git submodule init &&
git submodule update --recursive &&
git submodule add https://github.com/mksdev/libev-cmake &&
cd libev-cmake && rm .gitmodules &&
git submodule add https://github.com/enki/libev &&
cd .. &&
mkdir build && cd build && cmake .. && make