#!/usr/bin/env bash

# build native file dialog
pushd external/nativefiledialog/build/gmake_linux_zenity
make clean
# make CFLAGS=`pkg-config --cflags gtk+-3.0` LDFLAGS=`pkg-config --libs gtk+-3.0`
make
popd

rm -rf build
mkdir build
pushd build

echo ${1-'None specified'}

export CC=clang
export CXX=clang++
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=${1:-Release}
make -j10

cp compile_commands.json ../

popd
