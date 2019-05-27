#!/bin/bash

# Parse command line arguments
while [ $# -ne 0 ]
do
    arg="$1"
    case "$arg" in
        --analyze) IS_ANALYZE_BUILD=true;;
        *)         IS_UNKNOWN_ARG=true;;
    esac
    shift
done

CONFIG_TYPE=Release
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )
SOURCE_DIR=$SCRIPT_DIR/../..
OUTPUT_DIR=$SCRIPT_DIR/../Output/XCode
INSTALL_DIR=$OUTPUT_DIR/Install

if [ "$IS_ANALYZE_BUILD" == true ]; then
    BUILD_DIR=$OUTPUT_DIR/Analyze
    echo =========================================================
    echo Code analysis for build Methane $CONFIG_TYPE
    echo =========================================================
    echo  \* Build in:   $BUILD_DIR
    echo =========================================================
else
    BUILD_DIR=$OUTPUT_DIR/Build
    echo =========================================================
    echo Clean build and install Methane $CONFIG_TYPE
    echo =========================================================
    echo  \* Build in:   $BUILD_DIR
    echo  \* Install to: $INSTALL_DIR
    echo =========================================================
fi

rm -rf "$OUTPUT_DIR"
mkdir -p "$BUILD_DIR"

echo Pulling latest changes with submodules...
git pull --recurse-submodules

echo ---

if [ "$IS_ANALYZE_BUILD" == true ]; then
    echo Analyzing code with Sonar Scanner http://www.sonarcloud.io...
    build-wrapper-macosx-x86 --out-dir "$BUILD_DIR" \
        cmake -H"$SOURCE_DIR" -B"$BUILD_DIR" -G Xcode
    sonar-scanner \
        -Dsonar.projectKey=egorodet_MethaneKit \
        -Dsonar.organization=egorodet-github \
        -Dsonar.sources="$SOURCE_DIR" \
        -Dsonar.cfamily.build-wrapper-output="$BUILD_DIR" \
        -Dsonar.host.url=https://sonarcloud.io \
        -Dsonar.login=6e1dbce6af614f59d75f1d78f0609aaaa60caee1 \
else
    echo Building with XCode...
    cmake -H"$SOURCE_DIR" -B"$BUILD_DIR" -G Xcode -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"
    cmake --build "$BUILD_DIR" --config $CONFIG_TYPE --target install
fi