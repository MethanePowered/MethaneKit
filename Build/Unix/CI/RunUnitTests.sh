#!/bin/bash
# CI script to run all unit-test executables "*Test" from current directory
set +e
test_format="${1}"
extra_test_format="${2}"
result_ext='.xml'
test_results=''
result_error_level=0
echo Running unit-tests in directory $PWD
mkdir Results
mkdir Results/${test_format}
if [ ! -z "$extra_test_format" ]; then
    mkdir Results/${extra_test_format}
fi
for test_exe in *Test
do
  out_test_result=Results/${test_format}/$test_exe$result_ext
  if [ -z "$extra_test_format" ]; then
    ./$test_exe -r "${test_format}" -o "${out_test_result}"
  else
    out_extra_test_result=Results/${extra_test_format}/${test_exe}${result_ext}
    ./$test_exe -r "${test_format}::out=${out_test_result}" \
                -r "${extra_test_format}::out=${out_extra_test_result}"
  fi
  last_error_level=$?
  echo  - $test_exe - completed with $last_error_level exit status
  if [ $last_error_level != 0 ]; then
    result_error_level=$last_error_level
  fi
  if [ -f "$PWD/${out_test_result}" ]; then
    test_results+="$PWD/${out_test_result},"
  fi
done
echo "Test Result Files: $test_results"
echo "test_results=$test_results" >> $GITHUB_ENV
exit $result_error_level
