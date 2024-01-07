#!/bin/bash
# CI script to download Tracy release binary
tracy_version="${1}"
target_dir="${2}"
os_id=$(uname -s)
case ${os_id} in
  macOS|Darwin) os_name=MacOS ;;
  Linux) os_name=Ubuntu ;;
  Windows|MINGW*) os_name=Windows ;;
  *) echo "Unknown os: os" ; exit 7 ; ;;
esac
curl -sSLo Tracy.7z https://github.com/MethanePowered/Tracy/releases/download/v${tracy_version}/Tracy-${os_name}-v${tracy_version}.7z
7z x Tracy.7z -o${target_dir}
