#!/bin/bash
cd qgroundcontrol

SOURCE_DIR="/home/user/gstreamer-1.0-android-universal-1.18.6"
DEST_DIR="/home/user/qgroundcontrol/gstreamer-1.0-android-universal-1.18.6"

if [ -d "$DEST_DIR" ]; then
    echo "The directory '$DEST_DIR' already exists. Nothing to copy."
else
    echo "The directory '$DEST_DIR' does not exist. Copying from '$SOURCE_DIR'."
    cp -r "$SOURCE_DIR" "$DEST_DIR"
    echo "Directory copied successfully."
fi

if [ "$CLEAN" == "true" ]; then
    cd build-docker
    echo "---------------------------------------------------------------------------------------------"
    echo "----------------------------------------- Clean ---------------------------------------------"
    echo "---------------------------------------------------------------------------------------------"
    make clean
    cd ..
fi

if [ "$FAST" != "true" ]; then
    echo "---------------------------------------------------------------------------------------------"
    echo "--------------------------------------Update Custom------------------------------------------"
    echo "---------------------------------------------------------------------------------------------"
    echo ""
    cd custom
    python3 updateqrc.py
    cd ..
fi
echo "---------------------------------------------------------------------------------------------"
echo "---------------------------------------Build Start-------------------------------------------"
echo "---------------------------------------------------------------------------------------------"
echo ""
mkdir -p build-docker
cd build-docker

if [ "$FAST" != "true" ]; then
    qmake -r ../qgroundcontrol.pro -spec android-clang CONFIG+=release CONFIG+=installer ANDROID_ABIS="arm64-v8a"
fi
make -j$(nproc) apk