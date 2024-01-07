#!/bin/bash
# CI script to write build info file in install directory
repo_url="${1}"
branch_name="${2}"
commit_sha="${3}"
run_id="${4}"
config_preset="${5}"
build_preset="${6}"
runner_info="${7}"
build_info_file=$INSTALL_DIR/Build-Info.txt
echo Methane Kit v.${METHANE_VERSION_MAJOR}.${METHANE_VERSION_MINOR}.${METHANE_VERSION_PATCH}.${METHANE_VERSION_BUILD} > ${build_info_file}
echo   - Git repository: ${repo_url}, branch: ${branch_name}, commit SHA: ${commit_sha} >> {build_info_file}
echo   - GitHub Actions build url: ${repo_url}/actions/runs/${run_id} >> {build_info_file}
echo   - Built with CMake configure preset ${config_preset} and build preset ${build_preset} >> {build_info_file}
echo   - Built on agent ${runner_info}: >> {build_info_file}
echo   - Builder agent system information: >> {build_info_file}
runner_os=$(uname -s)
case $runner_os in
  Windows|MINGW*) systeminfo >> {build_info_file} ;;
  *) uname -a >> {build_info_file} ;;
esac
