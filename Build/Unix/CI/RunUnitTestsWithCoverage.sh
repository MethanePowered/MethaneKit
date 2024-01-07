#!/bin/bash
# CI script to run all unit-test executables "*Test" from current directory with code-coverage data collection
set +e
result_error_level=0
result_ext='.xml'
prof_data_ext='.profdata'
prof_raw_ext='.profraw'
lcov_ext='.lcov'
test_results=''
echo Running unit-tests and Converting LLVM code coverage data to lcov text format in directory $PWD
mkdir Results
mkdir Coverage
for test_exe in *Test
do
  ./$test_exe -r sonarqube -o "Results/$test_exe$result_ext"
  last_error_level=$?
  echo  - $test_exe - completed with $last_error_level exit status
  if [ $last_error_level != 0 ]; then
    result_error_level=$last_error_level
  fi
  if [ -f "$PWD/Results/$test_exe$result_ext" ]; then
    test_results+="$PWD/Results/$test_exe$result_ext,"
  fi
  if [ ! -f default.profraw ]; then
    continue
  fi
  mv default.profraw "$test_exe$prof_raw_ext"
  xcrun llvm-profdata merge -o "$test_exe$prof_data_ext" "$test_exe$prof_raw_ext"
  xcrun llvm-cov export -format lcov -instr-profile="$test_exe$prof_data_ext" -arch=x86_64 ./$test_exe > "./Coverage/$test_exe$lcov_ext"
  echo    - Converted code coverage from "$test_exe$prof_raw_ext" to lcov text format "./Coverage/$test_exe$lcov_ext", $? exit status
done
echo "Test Result Files: $test_results"
echo "test_results=$test_results" >> $GITHUB_ENV
echo List of generated coverage files in directory $PWD/Coverage
ls -la ./Coverage
exit $result_error_level
