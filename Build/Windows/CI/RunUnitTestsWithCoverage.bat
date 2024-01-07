REM CI script to run all unit-test executables "*Test.exe" from current directory with code-coverage data collection
chcp 65001 #set code page to utf-8
setlocal enabledelayedexpansion
set SOURCES_DIR=%1
set TESTS_POSIX_DIR=%2
set open_cpp_coverage_exe=OpenCppCoverage\OpenCppCoverage.exe
set test_results=
if not exist "%open_cpp_coverage_exe%" (
  echo File path "%open_cpp_coverage_exe%" does not exist!
  exit 101
)
echo Running unit-tests in directory "%cd%"
mkdir Results
mkdir Coverage
set /A result_error_level=0
for /r "." %%a in (*Test.exe) do (
  echo %open_cpp_coverage_exe% --sources "%SOURCES_DIR%" --export_type=cobertura:Coverage\%%~na.xml -- "%%~fa" -r sonarqube -o "Results\%%~na.xml"
  %open_cpp_coverage_exe% --sources "%SOURCES_DIR%" --export_type=cobertura:Coverage\%%~na.xml -- "%%~fa" -r sonarqube -o "Results\%%~na.xml"
  echo  - %%~na - completed with !errorlevel! exit status
  if not !errorlevel!==0 (
    set /A result_error_level=!errorlevel!
  )
  if .!test_results!==. (
    set test_results=%TESTS_POSIX_DIR%/Results/%%~na.xml
  ) else (
    set test_results=!test_results!,%TESTS_POSIX_DIR%/Results/%%~na.xml
  )
)
echo Test Result Files: %test_results%
echo test_results=%test_results%>> %GITHUB_ENV%
exit !result_error_level!
