#[[****************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: MethaneUtils.cmake
Cmake utility functions

*****************************************************************************]]

function(trim_spaces IN_STRING OUT_TRIMMED_STRING)
    string(REGEX REPLACE "^[ ]+" "" TRIMMED_STRING ${IN_STRING})
    string(REGEX REPLACE "[ ]+$" "" TRIMMED_STRING ${TRIMMED_STRING})
    set(${OUT_TRIMMED_STRING} ${TRIMMED_STRING} PARENT_SCOPE)
endfunction()

function(split_by_first_delimiter IN_STRING IN_DELIMITER OUT_LEFT_SUBSTRING OUT_RIGHT_SUBSTRING)
    string(REGEX MATCH "^[^${IN_DELIMITER}]+" LEFT_SUBSTRING ${IN_STRING})
    if(NOT LEFT_SUBSTRING STREQUAL IN_STRING)
        string(REPLACE "${LEFT_SUBSTRING}${IN_DELIMITER}" "" RIGHT_SUBSTRING ${IN_STRING})
    endif()
    set(${OUT_LEFT_SUBSTRING} ${LEFT_SUBSTRING} PARENT_SCOPE)
    set(${OUT_RIGHT_SUBSTRING} ${RIGHT_SUBSTRING} PARENT_SCOPE)
endfunction()

function(split_by_last_delimiter IN_STRING IN_DELIMITER OUT_LEFT_SUBSTRING OUT_RIGHT_SUBSTRING)
    string(REGEX MATCH "[^${IN_DELIMITER}]+$" RIGHT_SUBSTRING ${IN_STRING})
    if(NOT RIGHT_SUBSTRING STREQUAL IN_STRING)
        string(REPLACE "${IN_DELIMITER}${RIGHT_SUBSTRING}" "" LEFT_SUBSTRING ${IN_STRING})
    endif()
    set(${OUT_LEFT_SUBSTRING} ${LEFT_SUBSTRING} PARENT_SCOPE)
    set(${OUT_RIGHT_SUBSTRING} ${RIGHT_SUBSTRING} PARENT_SCOPE)
endfunction()

function(shorten_target_name IN_TARGET_NAME OUT_TARGET_NAME)
    # Visual Studio custom command targets may fail because of paths exceeding 256 symbols,
    # so we have to shorten long target names by replacing them with 32 symbol hashes
    string(LENGTH "${IN_TARGET_NAME}" IN_TARGET_LENGTH)
    if((CMAKE_GENERATOR MATCHES "^Visual Studio") AND (IN_TARGET_LENGTH GREATER 32))
        string(MD5 TARGET_NAME_HASH "${IN_TARGET_NAME}")
        set(${OUT_TARGET_NAME} ${TARGET_NAME_HASH} PARENT_SCOPE)
    else()
        set(${OUT_TARGET_NAME} ${IN_TARGET_NAME} PARENT_SCOPE)
    endif()
endfunction()

function(get_file_name FILE_PATH OUT_FILE_NAME)
    trim_spaces(${FILE_PATH} FILE_PATH)
    split_by_last_delimiter(${FILE_PATH} "." FILE_PATH_NO_EXT FILE_EXTENSION)
    string(REGEX MATCH "[^/\\]+$" FILE_NAME ${FILE_PATH_NO_EXT})
    set(${OUT_FILE_NAME} ${FILE_NAME} PARENT_SCOPE)
endfunction()

function(send_cmake_parse_errors FUNCTION_NAME PREFIX KEYWORDS_MISSING_VALUES UNPARSED_ARGUMENTS REQUIRED_ARGUMENTS)
    foreach(KEYWORD ${KEYWORDS_MISSING_VALUES})
        message(SEND_ERROR "Function '${FUNCTION_NAME}' argument '${KEYWORD}' values are missing")
    endforeach()

    foreach(ARGUMENT ${UNPARSED_ARGUMENTS})
        message(SEND_ERROR "Function '${FUNCTION_NAME}' argument '${ARGUMENT}' is unexpected")
    endforeach()

    foreach(ARGUMENT ${REQUIRED_ARGUMENTS})
        set(VAR_NAME ${PREFIX}_${ARGUMENT})
        if (NOT ${VAR_NAME})
            message(SEND_ERROR "Function '${FUNCTION_NAME}' argument '${ARGUMENT}' is required")
        endif()
    endforeach()
endfunction()