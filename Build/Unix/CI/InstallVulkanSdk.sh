#!/bin/bash
# CI script to download and install Vulkan SDK from MethanePowered/VulkanHeaders release downloads
sdk_ver="${1}"
sdk_dir="${2}"
runner_os=$(uname -s)
case $runner_os in
  macOS|Darwin) sdk_os=mac ; sdk_os_subdir=macOS ;;
  Linux) sdk_os=linux ; sdk_os_subdir=linux ;;
  Windows|MINGW*) sdk_os=windows ; sdk_os_subdir=windows ;;
  *) echo "Unknown runner_os: $runner_os" ; exit 7 ; ;;
esac
sdk_ver_dir="$sdk_dir/$sdk_ver/$sdk_os_subdir"
if [ -d "$sdk_ver_dir" ]; then
  echo "Vulkan SDK version directory already exists: $sdk_ver_dir"
else
  test -d $sdk_dir || mkdir -pv $sdk_dir
  vulkan_sdk_url=https://github.com/MethanePowered/VulkanHeaders/releases/download/vulkan-sdk-$sdk_ver/vulkan_sdk_$sdk_os.7z
  echo "Downloading Vulkan SDK archive $vulkan_sdk_url ..."
  curl -sSLo vulkan_sdk.7z $vulkan_sdk_url
  echo "Unpacking Vulkan SDK archive to $sdk_dir ..."
  7z x vulkan_sdk.7z -o$sdk_dir
  if [ ! -d $sdk_ver_dir ]; then
    echo "Vulkan SDK subdirectory not found: $sdk_ver_dir"
    exit 7
  fi
fi
echo "Setting VULKAN_SDK* environment variables..."
echo "VULKAN_SDK=$sdk_ver_dir" >> $GITHUB_ENV
echo "VULKAN_SDK_VERSION=$sdk_ver" >> $GITHUB_ENV
echo "VULKAN_SDK_PLATFORM=$sdk_os" >> $GITHUB_ENV
echo "PATH=$PATH:$sdk_ver_dir/bin" >> $GITHUB_ENV
echo "DYLD_LIBRARY_PATH=$sdk_ver_dir/lib:${DYLD_LIBRARY_PATH:-}" >> $GITHUB_ENV
