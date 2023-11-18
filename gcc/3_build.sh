#!/bin/sh

_BASE_DIR_=$PWD
. $_BASE_DIR_/cfg.sh

BIN_BU=$_BUILD_DIR_/$CROSS_TGT_NAME-binutils
if [ ! -d "$BIN_BU" ]; then
	mkdir -p $BIN_BU
fi

BIN_GCC=$_BUILD_DIR_/$CROSS_TGT_NAME-gcc
if [ ! -d "$BIN_GCC" ]; then
	mkdir -p $BIN_GCC
fi

CROSS_PREFIX="--prefix=$CROSS_PATH"
CROSS_OPTS="--target=$CROSS_TGT"\
" --disable-threads"\
" --disable-shared"\
" --with-system-zlib --with-isl"\
" --disable-__cxa_atexit"\
" --disable-libunwind-exceptions --enable-clocale=gnu"\
" --disable-libstdcxx-pch --disable-libssp"\
" --disable-nls"\
" --enable-plugin --disable-linker-build-id"\
" --enable-lto"\
" --enable-install-libiberty"\
" --with-linker-hash-style=both --with-gnu-ld"\
" --enable-gnu-indirect-function --disable-multilib --disable-werror"\
" --enable-checking=release"\
" --disable-default-pie"\
" --disable-default-ssp"\
" --enable-gnu-unique-object"

cd $BIN_BU
../binutils-*/configure $CROSS_PREFIX $CROSS_OPTS
make -j$(nproc)
make install
cd $_BASE_DIR_

cd $BIN_GCC
../gcc-*/configure --enable-languages=c,c++ $CROSS_PREFIX $CROSS_OPTS
make all-gcc -j$(nproc)
make install-gcc
cd $_BASE_DIR_

strip $CROSS_PATH/bin/*
strip $CROSS_PATH/libexec/gcc/$CROSS_TGT/*/*
