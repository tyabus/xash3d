#!/bin/bash

cd $TRAVIS_BUILD_DIR

#mkdir -p coverity && cd coverity/

export CC="gcc"
export CXX="g++"
export CFLAGS="-m32"
export CXXFLAGS="-m32"

#export COVERITY_SCAN_BRANCH_PATTERN="coverity_scan"
#export COVERITY_SCAN_NOTIFICATION_EMAIL="mr.maks0443@gmail.com"
#export COVERITY_SCAN_PROJECT_NAME="tyabus-xash3d"
#export COVERITY_SCAN_BUILD_COMMAND="make -j2 VERBOSE=1"

cmake \
	-DCMAKE_PREFIX_PATH=$TRAVIS_BUILD_DIR/sdl2-linux/usr/local \
	-DXASH_DOWNLOAD_DEPENDENCIES=yes \
	-DXASH_STATIC=ON \
	-DXASH_DLL_LOADER=ON \
	-DXASH_VGUI=ON \
	-DMAINUI_USE_STB=ON \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo .

bash $TRAVIS_BUILD_DIR/scripts/coverity_scan.sh
