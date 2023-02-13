#!/usr/bin/env bash

# build native file dialog

if [[ $OSTYPE == 'darwin'* ]];
then
    pushd external/nativefiledialog/build/gmake_macosx
    make clean
    # make CFLAGS=`pkg-config --cflags gtk+-3.0` LDFLAGS=`pkg-config --libs gtk+-3.0`
    make
    popd
else
    pushd external/nativefiledialog/build/gmake_linux_zenity
    make clean
    # make CFLAGS=`pkg-config --cflags gtk+-3.0` LDFLAGS=`pkg-config --libs gtk+-3.0`
    make
    popd
fi

rm -rf build
mkdir build
pushd build

echo ${1-'No build type specified'}

export CC=clang
export CXX=clang++
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=${1:-Release} -DwxWidgets_ROOT_DIR=${wx_ROOT:-$HOME/src/wxWidgets-3.2.2/} -Wno-dev
make -j10

cp compile_commands.json ../

popd
