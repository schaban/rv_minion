#!/bin/sh

_BASE_DIR_=$PWD
. $_BASE_DIR_/cfg.sh

if [ ! -d "$_BUILD_DIR_" ]; then
	mkdir -p $_BUILD_DIR_
fi

for mod in binutils gcc mpfr gmp mpc isl cloog
do
	echo "Extracting $mod..."
	tar -xf `ls $_ARC_DIR_/$mod-* | head -1` -C $_BUILD_DIR_
done

echo "Setting up lib links for binutils..."
cd $_BUILD_DIR_/binutils-*
for lib in mpfr gmp mpc isl
do
	ln -s ../$lib-* $lib
done
cd $_BASE_DIR_

echo "Setting up lib links for gcc..."
cd $_BUILD_DIR_/gcc-*
for lib in mpfr gmp mpc isl cloog
do
	ln -s ../$lib-* $lib
done
cd $_BASE_DIR_
