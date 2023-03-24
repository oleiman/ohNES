#!/usr/bin/env bash

# build native file dialog

for ARGUMENT in "$@"
do
    KEY=$(echo $ARGUMENT | cut -f1 -d=)

    KEY_LENGTH=${#KEY}
    VALUE="${ARGUMENT:$KEY_LENGTH+1}"

    export "$KEY"="$VALUE"
done

# if [[ $OSTYPE == 'darwin'* ]];
# then
#     pushd external/nativefiledialog/build/gmake_macosx
#     make clean
#     # make CFLAGS=`pkg-config --cflags gtk+-3.0` LDFLAGS=`pkg-config --libs gtk+-3.0`
#     make
#     popd
# else
#     pushd external/nativefiledialog/build/gmake_linux_zenity
#     make clean
#     # make CFLAGS=`pkg-config --cflags gtk+-3.0` LDFLAGS=`pkg-config --libs gtk+-3.0`
#     make
#     popd
# fi

rm -rf build
mkdir build
pushd build

echo "Build Type: ${mode-No build type specified}"
echo "WITH_APP=${app=ON}"
echo "WITH_TESTS=${test=OFF}"

export CC=clang
export CXX=clang++
cmake .. \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_BUILD_TYPE=${mode:-Release} \
      -DWITH_TESTS=${test=-OFF} \
      -DWITH_APP=${app=-ON} \
      -DwxWidgets_ROOT_DIR=${wx_ROOT:-$HOME/src/wxWidgets-3.2.2/} \
      -Wno-dev

make -j4

cp compile_commands.json ../

popd
