#!/bin/bash

CONFIG_TYPE=Release

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )
SOURCE_DIR=$SCRIPT_DIR/../..
OUTPUT_DIR=$SCRIPT_DIR/../Output/XCode
BUILD_DIR=$OUTPUT_DIR/Build
INSTALL_DIR=$OUTPUT_DIR/Install

echo =========================================================
echo Clean build and install Methane $CONFIG_TYPE
echo =========================================================
echo  \* Build in:   $BUILD_DIR
echo  \* Install to: $INSTALL_DIR
echo =========================================================

rm -rf "$OUTPUT_DIR"
mkdir -p "$BUILD_DIR"

echo Initializing submodules and pulling latest changes
git submodule update --init --depth 1 --recursive "$SOURCE_DIR"
git pull --recurse-submodules

cmake -H"$SOURCE_DIR" -B"$BUILD_DIR" -G Xcode -DCMAKE_BUILD_TYPE=$CONFIG_TYPE -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"
cmake --build "$BUILD_DIR" --config $CONFIG_TYPE --target install