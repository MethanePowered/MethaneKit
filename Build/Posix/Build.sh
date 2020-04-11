#!/bin/bash

# Parse command line arguments
while [ $# -ne 0 ]
do
    arg="$1"
    case "$arg" in
        --analyze) IS_ANALYZE_BUILD=true;;
        *)         echo "Unknown argument!" && exit 1;;
    esac
    shift
done

# Choose CMake generator depending on operating system
UNAME_OUT="$(uname -s)"
case "${UNAME_OUT}" in
    Linux*)     CMAKE_GENERATOR=Unix\ Makefiles;;
    Darwin*)    CMAKE_GENERATOR=Xcode;;
    *)          echo "Unsupported operating system!" 1>&2 && exit 1;;
esac

BUILD_TYPE=Release
BUILD_VERSION=0.4

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )
SOURCE_DIR=$SCRIPT_DIR/../..
OUTPUT_DIR=$SCRIPT_DIR/../Output/$CMAKE_GENERATOR
INSTALL_DIR=$OUTPUT_DIR/Install
CMAKE_FLAGS=" \
    -DMETHANE_SHADERS_CODEVIEW_ENABLED:BOOL=ON \
    -DMETHANE_RUN_TESTS_DURING_BUILD:BOOL=OFF \
    -DMETHANE_COMMAND_EXECUTION_LOGGING:BOOL=OFF \
    -DMETHANE_USE_OPEN_IMAGE_IO:BOOL=OFF \
    -DMETHANE_SCOPE_TIMERS_ENABLED:BOOL=OFF \
    -DMETHANE_TRACY_PROFILING_ENABLED:BOOL=OFF \
    -DMETHANE_ITT_INSTRUMENTATION_ENABLED:BOOL=ON"

if [ "$IS_ANALYZE_BUILD" == true ]; then

    BUILD_DIR=$OUTPUT_DIR/Analyze
    SONAR_SCANNER_DIR=$SOURCE_DIR/Externals/SonarScanner/binaries/MacOS
    SONAR_SCANNER_ZIP=$SONAR_SCANNER_DIR.zip
    SONAR_BUILD_WRAPPER_EXE=$SONAR_SCANNER_DIR/bin/build-wrapper-macosx-x86
    SONAR_SCANNER_EXE=$SONAR_SCANNER_DIR/bin/sonar-scanner

    echo =========================================================
    echo Code analysis for build Methane $BUILD_TYPE
    echo =========================================================
    echo  \* Build in:   $BUILD_DIR
    echo =========================================================
else
    BUILD_DIR=$OUTPUT_DIR/Build
    echo =========================================================
    echo Clean build and install Methane $BUILD_TYPE
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
        cmake -H"$SOURCE_DIR" -B"$BUILD_DIR" -G "$CMAKE_GENERATOR" $CMAKE_FLAGS

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
    echo Building with $CMAKE_GENERATOR...
    cmake -H"$SOURCE_DIR" -B"$BUILD_DIR" -G "$CMAKE_GENERATOR" -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" $CMAKE_FLAGS
    cmake --build "$BUILD_DIR" --config $BUILD_TYPE --target install

    echo Running unit-tests...
    cmake -E chdir "$BUILD_DIR" ctest --build-config $BUILD_TYPE --output-on-failure
fi