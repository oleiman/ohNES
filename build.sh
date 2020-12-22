#!/usr/bin/env bash

rm -rf build
mkdir build
pushd build

export CC=clang
export CXX=clang++
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
make -j10

popd
