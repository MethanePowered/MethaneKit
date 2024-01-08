#!/bin/bash
# CI script to write build info file in install directory
build_info_file=${1}
echo Methane Kit v.${METHANE_VERSION_MAJOR}.${METHANE_VERSION_MINOR}.${METHANE_VERSION_PATCH}.${METHANE_VERSION_BUILD} > ${build_info_file}
echo   - Git repository: ${REPO_URL}, branch: ${BRANCH_NAME}, commit SHA: ${COMMIT_SHA} >> ${build_info_file}
echo   - GitHub Actions build url: ${REPO_URL}/actions/runs/${RUN_ID} >> ${build_info_file}
echo   - Built with CMake configure preset ${CONFIG_PRESET} and build preset ${BUILD_PRESET} >> ${build_info_file}
echo   - Built on agent ${RUNNER_INFO}: >> ${build_info_file}
echo   - Builder agent system information: >> ${build_info_file}
runner_os=$(uname -s)
case $runner_os in
  Windows|MINGW*) systeminfo >> ${build_info_file} ;;
  *) uname -a >> ${build_info_file} ;;
esac
