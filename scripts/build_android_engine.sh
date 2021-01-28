#!/bin/bash

cd $TRAVIS_BUILD_DIR/xash3d-android-project
sh gen-version.sh
python2 scripts/makepak.py xash-extras assets/extras.pak
ndk-build -j2 APP_CFLAGS="-w" NDK_CCACHE=ccache
ln -s $TRAVIS_BUILD_DIR/xash3d-android-project/android/libs $TRAVIS_BUILD_DIR/xash3d-android-project/libs
ant debug -Dtest.version=1
cp bin/xash3d-debug.apk $TRAVIS_BUILD_DIR/xash3d-generic.apk
