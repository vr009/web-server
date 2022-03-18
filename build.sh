git submodule init &&
git submodule update --recursive &&
git submodule add https://github.com/mksdev/libev-cmake &&
mv cmake/CMakeLists.txt libev-cmake/CMakeLists.txt &&
mkdir build && cd build && cmake .. && make
