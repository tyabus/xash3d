#!/bin/bash

cd $TRAVIS_BUILD_DIR

export CC="gcc"
export CXX="g++"
export CFLAGS="-m32"
export CXXFLAGS="-m32"

cmake \
	-DCMAKE_PREFIX_PATH=$TRAVIS_BUILD_DIR/sdl2-linux/usr/local \
	-DXASH_DOWNLOAD_DEPENDENCIES=yes \
	-DXASH_STATIC=ON \
	-DXASH_DLL_LOADER=ON \
	-DXASH_VGUI=ON \
	-DMAINUI_USE_STB=ON \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo .

bash $TRAVIS_BUILD_DIR/scripts/coverity_scan.sh
