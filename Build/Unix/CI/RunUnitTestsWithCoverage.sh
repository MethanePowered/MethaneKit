#!/bin/bash
# CI script to run all unit-test executables "*Test" from current directory with code-coverage data collection
set +e
coverage_format="${1}"
test_format="${2}"
extra_test_format="${3}"
result_error_level=0
result_ext='.xml'
prof_data_ext='.profdata'
prof_raw_ext='.profraw'
lcov_ext='.lcov'
test_results=''
echo Running unit-tests and Converting LLVM code coverage data to lcov text format in directory $PWD
mkdir Results
mkdir Results/${test_format}
mkdir Results/${extra_test_format}
mkdir Coverage
for test_exe in *Test
do
  out_test_result=Results/${test_format}/${test_exe}${result_ext}
  out_extra_test_result=Results/${extra_test_format}/${test_exe}${result_ext}
  ./$test_exe -r "${test_format}::out=${out_test_result}" \
              -r "${extra_test_format}::out=${out_extra_test_result}"
  last_error_level=$?
  echo  - $test_exe - completed with $last_error_level exit status
  if [ $last_error_level != 0 ]; then
    result_error_level=$last_error_level
  fi
  if [ -f "$PWD/${out_test_result}" ]; then
    test_results+="$PWD/${out_test_result},"
  fi
  if [ ! -f default.profraw ]; then
    continue
  fi
  mv default.profraw "$test_exe$prof_raw_ext"
  xcrun llvm-profdata merge -o "$test_exe$prof_data_ext" "$test_exe$prof_raw_ext"
  xcrun llvm-cov export -format "${coverage_format}" -instr-profile="$test_exe$prof_data_ext" -arch=x86_64 ./$test_exe > "./Coverage/$test_exe$lcov_ext"
  echo    - Converted code coverage from "$test_exe$prof_raw_ext" to "${coverage_format}" text format "./Coverage/$test_exe$lcov_ext", $? exit status
done
echo "Test Result Files: $test_results"
echo "test_results=$test_results" >> $GITHUB_ENV
echo List of generated coverage files in directory $PWD/Coverage
ls -la ./Coverage
exit $result_error_level
