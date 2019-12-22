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
BUILD_VERSION=0.1

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )
SOURCE_DIR=$SCRIPT_DIR/../..
OUTPUT_DIR=$SCRIPT_DIR/../Output/XCode
INSTALL_DIR=$OUTPUT_DIR/Install
CMAKE_FLAGS='-DMETHANE_ITT_INSTRUMENTATION_ENABLED:BOOL=ON -DMETHANE_SHADERS_CODEVIEW_ENABLED:BOOL=ON -DMETHANE_RUN_TESTS_DURING_BUILD:BOOL=OFF'

if [ "$IS_ANALYZE_BUILD" == true ]; then

    BUILD_DIR=$OUTPUT_DIR/Analyze
    SONAR_SCANNER_DIR=$SOURCE_DIR/Externals/SonarScanner/binaries/MacOS
    SONAR_SCANNER_ZIP=$SONAR_SCANNER_DIR.zip
    SONAR_BUILD_WRAPPER_EXE=$SONAR_SCANNER_DIR/bin/build-wrapper-macosx-x86
    SONAR_SCANNER_EXE=$SONAR_SCANNER_DIR/bin/sonar-scanner

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

echo Pulling latest changes from submodules...
git submodule update --init --recursive

echo ---

if [ "$IS_ANALYZE_BUILD" == true ]; then

    echo Unpacking Sonar Scanner binaries...
    rm -rf "$SONAR_SCANNER_DIR"
    unzip -q "$SONAR_SCANNER_ZIP" -d "$SONAR_SCANNER_DIR"

    GITBRANCH=$(git symbolic-ref --short HEAD)

    echo Analyzing code with Sonar Scanner on branch $GITBRANCH...
    "$SONAR_BUILD_WRAPPER_EXE" --out-dir "$BUILD_DIR" \
        cmake -H"$SOURCE_DIR" -B"$BUILD_DIR" -G Xcode $CMAKE_FLAGS

    "$SONAR_SCANNER_EXE" \
        -Dsonar.projectKey=egorodet_MethaneKit \
        -Dsonar.organization=egorodet-github \
        -Dsonar.branch.name=$GITBRANCH \
        -Dsonar.projectVersion=$BUILD_VERSION \
        -Dsonar.sources="$SOURCE_DIR" \
        -Dsonar.cfamily.build-wrapper-output="$BUILD_DIR" \
        -Dsonar.host.url=https://sonarcloud.io \
        -Dsonar.login=6e1dbce6af614f59d75f1d78f0609aaaa60caee1

else
    echo Building with XCode...
    cmake -H"$SOURCE_DIR" -B"$BUILD_DIR" -G Xcode -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" $CMAKE_FLAGS
    cmake --build "$BUILD_DIR" --config $CONFIG_TYPE --target install

    echo Running unit-tests...
    cmake -E chdir "$BUILD_DIR" ctest --build-config $CONFIG_TYPE --output-on-failure
fi