#!/usr/bin/env bash

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
