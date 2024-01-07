#!/bin/bash
# CI script to build CMake preset
set -o pipefail
build_preset="${1}"
build_target="${2}"
log_file="${3}"
if [ "$build_preset" == "" ]; then
    echo "Build preset is undefined!"
    exit 7
fi
if [ "$build_target" == "" ]; then
    build_target="install"
fi
if [ "$log_file" == "" ]; then
    log_file="${BUILD_LOG_FILE}"
fi
# TSC Invariant check is disabled to allow running Catch test executables by CTest for tests list query in Profile builds
export TRACY_NO_INVARIANT_CHECK=1
cmake --build --preset "${build_preset}" --target "${build_target}" --parallel 4 2>&1 | tee -a "${log_file}"
