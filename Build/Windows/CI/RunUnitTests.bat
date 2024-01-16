REM CI script to run all unit-test executables "*Test.exe" from current directory
setlocal enabledelayedexpansion
set test_format=%1
echo Running unit-tests in directory "%cd%"
set /A result_error_level=0
mkdir Results
mkdir Results\%test_format%
for /r "." %%a in (*Test.exe) do (
  "%%~fa" -r %test_format% -o "%%~dpa\Results\%test_format%\%%~na.xml"
  echo  - %%~na - completed with !errorlevel! exit status
  if not !errorlevel!==0 (
    set /A result_error_level=!errorlevel!
  )
)
exit !result_error_level!
