# CI script to download and extract OpenCppCoverage release binaries from GitHub
Invoke-WebRequest -Uri 'https://github.com/MethanePowered/OpenCppCoverage/releases/download/release-0.9.9.0/OpenCppCoverage.zip' -OutFile 'OpenCppCoverage.zip'
Expand-Archive -Path 'OpenCppCoverage.zip' -DestinationPath 'OpenCppCoverage'
if (-not(Test-Path -Path 'OpenCppCoverage/OpenCppCoverage.exe' -PathType Leaf)) {
  Get-ChildItem 'OpenCppCoverage'
  throw 'OpenCppCoverage/OpenCppCoverage.exe executable was not found in unpacked content!'
}
