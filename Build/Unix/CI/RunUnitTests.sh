#!/bin/bash
# CI script to run all unit-test executables "*Test" from current directory
set +e
test_format="${1}"
result_ext='.xml'
test_results=''
result_error_level=0
echo Running unit-tests in directory $PWD
mkdir Results
for test_exe in *Test
do
  ./$test_exe -r "${test_format}" -o "Results/$test_exe$result_ext"
  last_error_level=$?
  echo  - $test_exe - completed with $last_error_level exit status
  if [ $last_error_level != 0 ]; then
    result_error_level=$last_error_level
  fi
  if [ -f "$PWD/Results/$test_exe$result_ext" ]; then
    test_results+="$PWD/Results/$test_exe$result_ext,"
  fi
done
echo "Test Result Files: $test_results"
echo "test_results=$test_results" >> $GITHUB_ENV
exit $result_error_level
