#[[****************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: MethaneShaders.cmake
Cross-platform HLSL shaders compilation and embedding into application resources
with transformation to Metal shaders using SPIRV tools.

*****************************************************************************]]

include(MethaneUtils)
include(MethaneModules)
include(CMakeRC)

function(get_shader_profile SHADER_TYPE PROFILE_VER OUT_PROFILE)
    if (SHADER_TYPE STREQUAL "frag")
        set(_PROFILE_TYPE ps)
    elseif(SHADER_TYPE STREQUAL "vert")
        set(_PROFILE_TYPE vs)
    else()
        message(FATAL_ERROR "Unsupported shader type: " ${SHADER_TYPE})
    endif()
    set(${OUT_PROFILE} "${_PROFILE_TYPE}_${PROFILE_VER}" PARENT_SCOPE)
endfunction()

function(get_target_shaders_dir FOR_TARGET TARGET_SHADERS_DIR)
    set(${TARGET_SHADERS_DIR} "${CMAKE_CURRENT_BINARY_DIR}/Shaders/${FOR_TARGET}" PARENT_SCOPE)
endfunction()

function(get_shaders_config SHADERS_HLSL SHADERS_CONFIG)
    trim_spaces(${SHADERS_HLSL} SHADERS_HLSL)
    split_by_last_delimiter(${SHADERS_HLSL} "." SHADERS_PATH FILE_EXT)
    set(${SHADERS_CONFIG} "${SHADERS_PATH}.cfg" PARENT_SCOPE)
endfunction()

function(get_metal_library FOR_TARGET SHADERS_HLSL METAL_LIBRARY)
    get_target_shaders_dir(${FOR_TARGET} TARGET_SHADERS_DIR)
    get_file_name(${SHADERS_HLSL} SHADERS_NAME)
    set(${METAL_LIBRARY} "${TARGET_SHADERS_DIR}/${SHADERS_NAME}.metallib" PARENT_SCOPE)
endfunction()

function(get_generated_shaders FOR_TARGET SHADERS_CONFIG SHADER_EXT SHADERS_GENERATED)
    get_target_shaders_dir(${FOR_TARGET} TARGET_SHADERS_DIR)
    get_file_name(${SHADERS_CONFIG} SHADERS_NAME)

    file(STRINGS ${SHADERS_CONFIG} CONFIG_STRINGS)
    foreach(KEY_VALUE_STRING ${CONFIG_STRINGS})
        trim_spaces(${KEY_VALUE_STRING} KEY_VALUE_STRING)
        split_by_first_delimiter(${KEY_VALUE_STRING} "=" SHADER_TYPE ENTRY_POINT_WITH_DEFINES)

        string(REGEX REPLACE ":| " "_" NEW_ENTRY_POINT ${ENTRY_POINT_WITH_DEFINES})
        string(REPLACE "=" "" NEW_ENTRY_POINT "${NEW_ENTRY_POINT}")
        set(NEW_ENTRY_POINT "${SHADERS_NAME}_${NEW_ENTRY_POINT}")

        list(APPEND _SHADERS_GENERATED "${TARGET_SHADERS_DIR}/${NEW_ENTRY_POINT}.${SHADER_EXT}")
    endforeach()

    set(${SHADERS_GENERATED} ${_SHADERS_GENERATED} PARENT_SCOPE)
endfunction()

function(generate_metal_shaders_from_hlsl FOR_TARGET SHADERS_HLSL PROFILE_VER OUT_SHADERS_METAL)
    get_platform_dir()
    get_target_shaders_dir(${FOR_TARGET} TARGET_SHADERS_DIR)
    get_file_name(${SHADERS_HLSL} SHADERS_NAME)
    get_shaders_config(${SHADERS_HLSL} SHADERS_CONFIG)

    set(DXC_BIN_DIR "${CMAKE_SOURCE_DIR}/Externals/DirectXCompiler/binaries/${PLATFORM_DIR}")
    set(DXC_EXE     "${DXC_BIN_DIR}/dxc")

    set(SPIRV_BIN_DIR      "${CMAKE_SOURCE_DIR}/Externals/SPIRV/binaries/${PLATFORM_DIR}")
    set(SPIRV_GEN_EXE      "${SPIRV_BIN_DIR}/glslangValidator")
    set(SPIRV_CROSS_EXE    "${SPIRV_BIN_DIR}/spirv-cross")

    file(STRINGS ${SHADERS_CONFIG} CONFIG_STRINGS)
    foreach(KEY_VALUE_STRING ${CONFIG_STRINGS})
        trim_spaces(${KEY_VALUE_STRING} KEY_VALUE_STRING)
        split_by_first_delimiter(${KEY_VALUE_STRING} "=" SHADER_TYPE ENTRY_POINT_WITH_DEFINES)
        split_by_first_delimiter(${ENTRY_POINT_WITH_DEFINES} ":" OLD_ENTRY_POINT SHADER_DEFINITIONS)

        set(NEW_ENTRY_POINT "${SHADERS_NAME}_${OLD_ENTRY_POINT}")
        string(REPLACE " " ";" SHADER_DEFINITIONS "${SHADER_DEFINITIONS}")

        set(SHADER_DEFINITION_ARGUMENTS )
        foreach(SHADER_DEFINITION ${SHADER_DEFINITIONS})
            list(APPEND SHADER_DEFINITION_ARGUMENTS -D${SHADER_DEFINITION})
            string(REPLACE "=" "" SHADER_DEFINITION_NAME ${SHADER_DEFINITION})
            set(NEW_ENTRY_POINT "${NEW_ENTRY_POINT}_${SHADER_DEFINITION_NAME}")
        endforeach()

        set(SHADER_SPIRV_FILE "${NEW_ENTRY_POINT}.spv")
        set(SHADER_METAL_FILE "${NEW_ENTRY_POINT}.metal")
        set(SHADER_SPIRV_PATH "${TARGET_SHADERS_DIR}/${SHADER_SPIRV_FILE}")
        set(SHADER_METAL_PATH "${TARGET_SHADERS_DIR}/${SHADER_METAL_FILE}")
        set(GENERATE_METAL_TARGET ${FOR_TARGET}_Generate_${SHADER_METAL_FILE})

        get_shader_profile(${SHADER_TYPE} ${PROFILE_VER} SHADER_PROFILE)

        add_custom_target(${GENERATE_METAL_TARGET}
            COMMENT "Generating Metal shader source code from HLSL " ${NEW_ENTRY_POINT} " for application " ${TARGET}
            BYPRODUCTS "${SHADER_METAL_PATH}"
            DEPENDS ${SHADERS_HLSL} ${SHADERS_CONFIG}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGET_SHADERS_DIR}"
            COMMAND ${DXC_EXE} -spirv -T ${SHADER_PROFILE} -E ${OLD_ENTRY_POINT} ${SHADER_DEFINITION_ARGUMENTS} "${SHADERS_HLSL}" -Fo "${SHADER_SPIRV_PATH}"
            #COMMAND ${SPIRV_GEN_EXE} --hlsl-iomap -S ${SHADER_TYPE} -e ${OLD_ENTRY_POINT} ${SHADER_DEFINITION_ARGUMENTS} -o "${SHADER_SPIRV_PATH}" -V -D "${SHADERS_HLSL}"
            COMMAND ${SPIRV_CROSS_EXE} --msl --msl-version 020101 --msl-decoration-binding --rename-entry-point ${OLD_ENTRY_POINT} ${NEW_ENTRY_POINT} ${SHADER_TYPE} --output "${SHADER_METAL_PATH}" "${SHADER_SPIRV_PATH}"
        )

        set_target_properties(${GENERATE_METAL_TARGET}
            PROPERTIES
            FOLDER "Build/${FOR_TARGET}/Shaders"
        )

        add_dependencies(${GENERATE_METAL_TARGET} DirectXCompilerUnpack-build)
        add_dependencies(${GENERATE_METAL_TARGET} SPIRV-build)
        add_dependencies(${FOR_TARGET} ${GENERATE_METAL_TARGET})

        list(APPEND SHADERS_METAL ${SHADER_METAL_PATH})
    endforeach()

    set(${OUT_SHADERS_METAL} ${SHADERS_METAL} PARENT_SCOPE)
endfunction()

function(compile_metal_shaders_to_library FOR_TARGET SDK METAL_SHADERS METAL_LIBRARY)
    get_target_shaders_dir(${FOR_TARGET} TARGET_SHADERS_DIR)

    if(METHANE_SHADERS_CODEVIEW_ENABLED OR CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(EXTRA_COMPILE_FLAGS -gline-tables-only -MO)
    endif()

    foreach(SHADER_METAL_PATH ${METAL_SHADERS})
        string(REPLACE "/" ";" SHADER_METAL_PATH_LIST "${SHADER_METAL_PATH}")
        list(LENGTH SHADER_METAL_PATH_LIST SHADER_METAL_PATH_LIST_LENGTH)
        math(EXPR SHADER_METAL_PATH_LIST_LAST_INDEX "${SHADER_METAL_PATH_LIST_LENGTH} - 1")
        list(GET SHADER_METAL_PATH_LIST ${SHADER_METAL_PATH_LIST_LAST_INDEX} SHADER_METAL_FILE)

        set(SHADER_AIR_PATH ${SHADER_METAL_PATH}.air)
        set(SHADER_COMPILE_TARGET ${FOR_TARGET}_Compile_${SHADER_METAL_FILE})

        add_custom_target(${SHADER_COMPILE_TARGET}
            COMMENT "Compiling Metal shader " ${SHADER_METAL_FILE} " for target " ${TARGET}
            BYPRODUCTS "${SHADER_AIR_PATH}"
            DEPENDS "${SHADER_METAL_PATH}"
            COMMAND xcrun -sdk ${SDK} metal ${EXTRA_COMPILE_FLAGS} -c "${SHADER_METAL_PATH}" -o "${SHADER_AIR_PATH}"
        )

        set_target_properties(${SHADER_COMPILE_TARGET}
            PROPERTIES
            FOLDER "Build/${FOR_TARGET}/Shaders"
        )
        
        add_dependencies(${SHADER_COMPILE_TARGET} ${FOR_TARGET}_Generate_${SHADER_METAL_FILE})

        list(APPEND _SHADERS_AIR_FILES "${SHADER_AIR_PATH}")
        list(APPEND _SHADER_COMPILE_TARGETS ${SHADER_COMPILE_TARGET})
    endforeach()

    get_file_name(${METAL_LIBRARY} METAL_LIBRARY_NAME)
    set(METAL_LIB_TARGET ${FOR_TARGET}_LinkMetalLibrary_${METAL_LIBRARY_NAME})
    add_custom_target(${METAL_LIB_TARGET}
        COMMENT "Linking default Metal library for application " ${TARGET}
        BYPRODUCTS "${METAL_LIBRARY}"
        DEPENDS "${_SHADERS_AIR_FILES}"
        COMMAND xcrun -sdk ${SDK} metallib ${_SHADERS_AIR_FILES} -o "${METAL_LIBRARY}"
    )

    set_target_properties(${METAL_LIB_TARGET}
        PROPERTIES
        FOLDER "Build/${FOR_TARGET}/Shaders"
    )

    add_dependencies(${METAL_LIB_TARGET} ${_SHADER_COMPILE_TARGETS})
    add_dependencies(${FOR_TARGET} ${METAL_LIB_TARGET})
endfunction()

function(compile_hlsl_shaders FOR_TARGET SHADERS_HLSL PROFILE_VER OUT_COMPILED_SHADER_BINS)
    
    get_platform_dir()
    get_target_arch(WIN_ARCH)
    get_target_shaders_dir(${FOR_TARGET} TARGET_SHADERS_DIR)
    get_file_name(${SHADERS_HLSL} SHADERS_NAME)
    get_shaders_config(${SHADERS_HLSL} SHADERS_CONFIG)

    set(SHADER_COMPILER_DIR "${CMAKE_SOURCE_DIR}/Externals/DirectXCompiler/binaries/${PLATFORM_DIR}-${WIN_ARCH}/bin")
    set(SHADER_COMPILER_EXE "${SHADER_COMPILER_DIR}/dxc.exe")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(EXTRA_COMPILE_FLAGS /Od)
    else()
        set(EXTRA_COMPILE_FLAGS /O3 /Gfa /all_resources_bound)
    endif()

    if(METHANE_SHADERS_CODEVIEW_ENABLED OR CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(EXTRA_COMPILE_FLAGS ${EXTRA_COMPILE_FLAGS} /Zi /Qembed_debug)
    endif()

    file(STRINGS ${SHADERS_CONFIG} CONFIG_STRINGS)
    foreach(KEY_VALUE_STRING ${CONFIG_STRINGS})
        trim_spaces(${KEY_VALUE_STRING} KEY_VALUE_STRING)
        split_by_first_delimiter(${KEY_VALUE_STRING} "=" SHADER_TYPE ENTRY_POINT_WITH_DEFINES)
        split_by_first_delimiter(${ENTRY_POINT_WITH_DEFINES} ":" ORIG_ENTRY_POINT SHADER_DEFINITIONS)

        set(NEW_ENTRY_POINT ${ORIG_ENTRY_POINT})
        string(REPLACE " " ";" SHADER_DEFINITIONS "${SHADER_DEFINITIONS}")

        set(SHADER_DEFINITION_ARGUMENTS)
        foreach(SHADER_DEFINITION ${SHADER_DEFINITIONS})
            list(APPEND SHADER_DEFINITION_ARGUMENTS /D ${SHADER_DEFINITION})
            string(REPLACE "=" "" SHADER_DEFINITION_NAME ${SHADER_DEFINITION})
            set(NEW_ENTRY_POINT "${NEW_ENTRY_POINT}_${SHADER_DEFINITION_NAME}")
        endforeach()

        get_shader_profile(${SHADER_TYPE} ${PROFILE_VER} SHADER_PROFILE)

        set(SHADER_OBJ_FILE "${SHADERS_NAME}_${NEW_ENTRY_POINT}.obj")
        set(SHADER_OBJ_PATH "${TARGET_SHADERS_DIR}/${SHADER_OBJ_FILE}")

        list(APPEND _OUT_COMPILED_SHADER_BINS ${SHADER_OBJ_PATH})

        shorten_target_name(${FOR_TARGET}_HLSL_${NEW_ENTRY_POINT} COMPILE_SHADER_TARGET)
        add_custom_target(${COMPILE_SHADER_TARGET}
            COMMENT "Compiling HLSL shader from file " ${SHADERS_HLSL} " with profile " ${SHADER_PROFILE} " and macro-definition" ${SHADER_DEFINITIONS} "to OBJ file " ${SHADER_OBJ_FILE}
            BYPRODUCTS "${SHADER_OBJ_PATH}"
            DEPENDS ${SHADERS_HLSL} ${SHADERS_CONFIG}
            WORKING_DIRECTORY "${SHADER_COMPILER_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGET_SHADERS_DIR}"
            COMMAND ${SHADER_COMPILER_EXE} /T ${SHADER_PROFILE} /E ${ORIG_ENTRY_POINT} /Fo ${SHADER_OBJ_PATH} ${EXTRA_COMPILE_FLAGS} ${SHADER_DEFINITION_ARGUMENTS} ${SHADERS_HLSL}
        )

        add_dependencies(${COMPILE_SHADER_TARGET} DirectXCompilerUnpack-build)

        set_target_properties(${COMPILE_SHADER_TARGET}
            PROPERTIES
            FOLDER "Build/${FOR_TARGET}/Shaders"
        )

        add_dependencies(${FOR_TARGET}_Shaders ${COMPILE_SHADER_TARGET})
    endforeach()

    set(${OUT_COMPILED_SHADER_BINS} ${_OUT_COMPILED_SHADER_BINS} PARENT_SCOPE)
endfunction()

function(add_methane_shaders TARGET HLSL_SOURCES PROFILE_VER)

    set(RESOURCE_NAMESPACE ${TARGET})

    if (WIN32)

        foreach(SHADERS_HLSL ${HLSL_SOURCES})
            get_shaders_config(${SHADERS_HLSL} SHADERS_CONFIG)
            get_generated_shaders(${TARGET} "${SHADERS_CONFIG}" "obj" SHADERS_OBJ)
            list(APPEND SHADERS_OBJ_FILES ${SHADERS_OBJ})
            list(APPEND CONFIG_SOURCES ${SHADERS_CONFIG})
        endforeach()
        
        get_target_shaders_dir(${TARGET} TARGET_SHADERS_DIR)

        set(SHADER_RESOURCES_TARGET ${TARGET}_Shaders)
        cmrc_add_resource_library(${SHADER_RESOURCES_TARGET}
            ALIAS Methane::Resources::Shaders
            WHENCE "${TARGET_SHADERS_DIR}"
            NAMESPACE ${RESOURCE_NAMESPACE}::Shaders
            ${SHADERS_OBJ_FILES}
        )

        foreach(SHADERS_HLSL ${HLSL_SOURCES})
            compile_hlsl_shaders(${TARGET} ${SHADERS_HLSL} ${PROFILE_VER} OUT_COMPILED_SHADER_BINS)
        endforeach()

        target_sources(${TARGET} PRIVATE
            ${HLSL_SOURCES}
            ${CONFIG_SOURCES}
        )

        target_link_libraries(${TARGET}
            ${SHADER_RESOURCES_TARGET}
        )

        # Disable automatic HLSL shaders compilation in Visual Studio, since it's compiled by custom target
        set_source_files_properties(${HLSL_SOURCES}
            PROPERTIES
            VS_TOOL_OVERRIDE "None"
        )

        set_target_properties(${SHADER_RESOURCES_TARGET}
            PROPERTIES
            FOLDER "Build/${TARGET}/Resources"
        )

        target_compile_definitions(${TARGET}
            PRIVATE
            SHADER_RESOURCE_NAMESPACE=${RESOURCE_NAMESPACE}::Shaders
        )

    elseif(APPLE)

        # Get list of generated Metal shaders
        foreach(SHADERS_HLSL ${HLSL_SOURCES})
            get_shaders_config(${SHADERS_HLSL} SHADERS_CONFIG)
            list(APPEND CONFIG_SOURCES ${SHADERS_CONFIG})

            get_metal_library(${TARGET} ${SHADERS_CONFIG} METAL_LIBRARY)
            list(APPEND METAL_LIBRARIES ${METAL_LIBRARY})

            get_generated_shaders(${TARGET} ${SHADERS_CONFIG} "metal" METAL_SOURCES)
            list(APPEND GENERATED_SOURCES ${METAL_SOURCES})
        endforeach()

        target_sources(${TARGET} PRIVATE
            ${HLSL_SOURCES}
            ${CONFIG_SOURCES}
            ${GENERATED_SOURCES}
            ${METAL_LIBRARIES}
        )

        # Set bundle location for metal library files
        set_source_files_properties(
            ${METAL_LIBRARIES}
            PROPERTIES MACOSX_PACKAGE_LOCATION
            "Resources"
        )

        set_source_files_properties(
            ${GENERATED_SOURCES}
            ${METAL_LIBRARIES}
            PROPERTIES GENERATED
            TRUE
        )

        source_group("Generated Shaders" FILES
            ${GENERATED_SOURCES}
            ${METAL_LIBRARIES}
        )

        # Add Metal libraries to the list of prerequisites to auto-copy them to the application Resources
        set_target_properties(${TARGET}
            PROPERTIES PREREQUISITE_RESOURCES
            "${METAL_LIBRARIES}"
        )

        # Generate Metal shaders from HLSL sources with SPIRV toolset and compile to Metal Library

        foreach(SHADERS_HLSL ${HLSL_SOURCES})
            set(SHADERS_METAL) # init with empty list
            get_metal_library(${TARGET} ${SHADERS_HLSL} METAL_LIBRARY)
            generate_metal_shaders_from_hlsl(${TARGET} ${SHADERS_HLSL} ${PROFILE_VER} SHADERS_METAL)
            compile_metal_shaders_to_library(${TARGET} "macosx" "${SHADERS_METAL}" "${METAL_LIBRARY}")
        endforeach()

    endif()

    source_group("Source Shaders" FILES
        ${HLSL_SOURCES}
        ${CONFIG_SOURCES}
    )

endfunction()