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
    -DMETHANE_COMMAND_DEBUG_GROUPS_ENABLED:BOOL=ON \
    -DMETHANE_LOGGING_ENABLED:BOOL=OFF \
    -DMETHANE_USE_OPEN_IMAGE_IO:BOOL=OFF \
    -DMETHANE_SCOPE_TIMERS_ENABLED:BOOL=OFF \
    -DMETHANE_ITT_INSTRUMENTATION_ENABLED:BOOL=ON \
    -DMETHANE_GPU_INSTRUMENTATION_ENABLED:BOOL=OFF \
    -DMETHANE_TRACY_PROFILING_ENABLED:BOOL=OFF \
    -DMETHANE_TRACY_PROFILING_ON_DEMAND:BOOL=OFF"

GRAPHVIZ_DIR=$OUTPUT_DIR/GraphViz
GRAPHVIZ_DOT_DIR=$GRAPHVIZ_DIR/dot
GRAPHVIZ_IMG_DIR=$GRAPHVIZ_DIR/img
GRAPHVIZ_FILE=MethaneKit.dot
GRAPHVIZ_DOT_EXE=$(which dot)

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
if [ $? -ne 0 ]; then
    echo "Failed to create clean build directory."
    exit 1
fi

if [ "$IS_ANALYZE_BUILD" == true ]; then

    echo Unpacking Sonar Scanner binaries...
    rm -rf "$SONAR_SCANNER_DIR"
    unzip -q "$SONAR_SCANNER_ZIP" -d "$SONAR_SCANNER_DIR"
    if [ $? -ne 0 ]; then
        echo "Sonar-scanner binaries unpack failed."
        exit 1
    fi

    GITBRANCH=$(git symbolic-ref --short HEAD)

    echo Analyzing code with Sonar Scanner on branch $GITBRANCH...
    "$SONAR_BUILD_WRAPPER_EXE" --out-dir "$BUILD_DIR" \
        cmake -H"$SOURCE_DIR" -B"$BUILD_DIR" -G "$CMAKE_GENERATOR" $CMAKE_FLAGS
    if [ $? -ne 0 ]; then
        echo "Sonar-scanner CMake generation failed."
        exit 1
    fi

    "$SONAR_SCANNER_EXE" \
        -Dsonar.projectKey=egorodet_MethaneKit \
        -Dsonar.organization=egorodet-github \
        -Dsonar.branch.name=$GITBRANCH \
        -Dsonar.projectVersion=$BUILD_VERSION \
        -Dsonar.sources="$SOURCE_DIR" \
        -Dsonar.cfamily.build-wrapper-output="$BUILD_DIR" \
        -Dsonar.host.url=https://sonarcloud.io \
        -Dsonar.login=6e1dbce6af614f59d75f1d78f0609aaaa60caee1
    if [ $? -ne 0 ]; then
        echo "Sonar-scanner build failed."
        exit 1
    fi

else

    echo ----------
    echo Generating build files for $CMAKE_GENERATOR...
    cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -G "$CMAKE_GENERATOR" --graphviz="$GRAPHVIZ_DOT_DIR/$GRAPHVIZ_FILE" -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" $CMAKE_FLAGS
    if [ $? -ne 0 ]; then
        echo "Methane CMake generation failed."
        exit 1
    fi

    echo ----------
    if [ -x "$GRAPHVIZ_DOT_EXE" ]; then
        echo Converting GraphViz diagrams to images...
        mkdir "$GRAPHVIZ_IMG_DIR"
        for DOT_PATH in $GRAPHVIZ_DOT_DIR/*; do
            DOT_IMG=$GRAPHVIZ_IMG_DIR/${DOT_PATH##*/}.png
            echo Writing image $DOT_IMG...
            "$GRAPHVIZ_DOT_EXE" -Tpng "$DOT_PATH" -o "$DOT_IMG"
            if [ $? -ne 0 ]; then
                echo "Dot failed to generate diagram image."
                exit 1
            fi
        done
    else
        echo GraphViz dot executable was not found. Skipping graph images generation.
    fi

    echo ----------
    echo Build with $CMAKE_GENERATOR...
    cmake --build "$BUILD_DIR" --config $BUILD_TYPE --target install
    if [ $? -ne 0 ]; then
        echo "Methane build failed."
        exit 1
    fi

    echo ----------
    echo Running unit-tests...
    cmake -E chdir "$BUILD_DIR" ctest --build-config $BUILD_TYPE --output-on-failure
    if [ $? -ne 0 ]; then
        echo "Methane tests failed."
        exit 1
    fi

fi