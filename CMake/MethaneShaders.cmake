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

function(get_metal_library FOR_TARGET SHADERS_HLSL METAL_LIBRARY)
    get_target_shaders_dir(${FOR_TARGET} TARGET_SHADERS_DIR)
    get_file_name(${SHADERS_HLSL} SHADERS_NAME)
    set(${METAL_LIBRARY} "${TARGET_SHADERS_DIR}/${SHADERS_NAME}.metallib" PARENT_SCOPE)
endfunction()

function(get_generated_shader_extension OUT_SHADER_EXT)
    if(METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX)
        set(${OUT_SHADER_EXT} "obj" PARENT_SCOPE)
    elseif(METHANE_GFX_API EQUAL METHANE_GFX_METAL)
        set(${OUT_SHADER_EXT} "metal" PARENT_SCOPE)
    elseif(METHANE_GFX_API EQUAL METHANE_GFX_VULKAN)
        set(${OUT_SHADER_EXT} "spirv" PARENT_SCOPE)
    endif()
endfunction()

function(generate_metal_shaders_from_hlsl FOR_TARGET SHADERS_HLSL PROFILE_VER SHADER_TYPES OUT_SHADERS_METAL OUT_GENERATE_METAL_TARGETS)
    get_platform_dir(PLATFORM_DIR CPP_EXT)
    get_target_shaders_dir(${FOR_TARGET} TARGET_SHADERS_DIR)
    get_file_name(${SHADERS_HLSL} SHADERS_NAME)

    set(DXC_BIN_DIR "${CMAKE_SOURCE_DIR}/Externals/DirectXCompiler/binaries/${PLATFORM_DIR}/bin")
    set(DXC_EXE     "${DXC_BIN_DIR}/dxc")

    set(SPIRV_BIN_DIR   "${CMAKE_SOURCE_DIR}/Externals/SPIRV/binaries/${PLATFORM_DIR}")
    set(SPIRV_GEN_EXE   "${SPIRV_BIN_DIR}/glslangValidator")
    set(SPIRV_CROSS_EXE "${SPIRV_BIN_DIR}/spirv-cross")

    foreach(KEY_VALUE_STRING ${SHADER_TYPES})
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
            DEPENDS "${SHADERS_HLSL}"
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
        list(APPEND GENERATE_METAL_TARGETS ${GENERATE_METAL_TARGET})
    endforeach()

    set(${OUT_SHADERS_METAL} ${SHADERS_METAL} PARENT_SCOPE)
    set(${OUT_GENERATE_METAL_TARGETS} ${GENERATE_METAL_TARGETS} PARENT_SCOPE)
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

function(compile_hlsl_shaders FOR_TARGET SHADERS_HLSL PROFILE_VER SHADER_TYPES OUT_COMPILED_SHADER_BINARIES OUT_COMPILE_SHADER_TARGETS)

    get_platform_arch_dir(PLATFORM_ARCH_DIR CPP_EXT)
    get_target_shaders_dir(${FOR_TARGET} TARGET_SHADERS_DIR)
    get_file_name(${SHADERS_HLSL} SHADERS_NAME)
    get_generated_shader_extension(OUTPUT_FILE_EXT)

    set(DXC_DIR "${CMAKE_SOURCE_DIR}/Externals/DirectXCompiler/binaries/${PLATFORM_ARCH_DIR}/bin")
    set(DXC_EXE "${DXC_DIR}/dxc")

    if (NOT WIN32)
        set(DXC_EXE "LD_LIBRARY_PATH=.;${DXC_EXE}")
    endif()

    if (METHANE_GFX_API EQUAL METHANE_GFX_VULKAN)
        set(OUTPUT_TYPE_ARG -spirv -fspv-reflect)
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(EXTRA_COMPILE_FLAGS /Od)
    else()
        set(EXTRA_COMPILE_FLAGS /O3 /Gfa /all_resources_bound)
    endif()

    if(METHANE_SHADERS_CODEVIEW_ENABLED OR CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(EXTRA_COMPILE_FLAGS ${EXTRA_COMPILE_FLAGS} /Zi /Qembed_debug)
    endif()

    foreach(KEY_VALUE_STRING ${SHADER_TYPES})
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

        if(SHADER_TYPE STREQUAL "vert")
            set(EXTRA_OPTIONS -fvk-invert-y)
        else()
            set(EXTRA_OPTIONS "")
        endif()

        set(SHADER_OBJ_FILE "${SHADERS_NAME}_${NEW_ENTRY_POINT}.${OUTPUT_FILE_EXT}")
        set(SHADER_OBJ_PATH "${TARGET_SHADERS_DIR}/${SHADER_OBJ_FILE}")

        shorten_target_name(${FOR_TARGET}_HLSL_${NEW_ENTRY_POINT} COMPILE_SHADER_TARGET)
        add_custom_target(${COMPILE_SHADER_TARGET}
            COMMENT "Compiling HLSL shader from file ${SHADERS_HLSL} with profile ${SHADER_PROFILE} and macro-definitions \"${SHADER_DEFINITIONS}\" to ${OUTPUT_FILE_EXT} file ${SHADER_OBJ_FILE}"
            BYPRODUCTS "${SHADER_OBJ_PATH}"
            DEPENDS "${SHADERS_HLSL}" "${SHADERS_CONFIG}"
            WORKING_DIRECTORY "${DXC_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGET_SHADERS_DIR}"
            COMMAND ${DXC_EXE} ${OUTPUT_TYPE_ARG} ${EXTRA_OPTIONS} /T ${SHADER_PROFILE} /E ${ORIG_ENTRY_POINT} /Fo ${SHADER_OBJ_PATH} ${EXTRA_COMPILE_FLAGS} ${SHADER_DEFINITION_ARGUMENTS} ${SHADERS_HLSL}
        )

        add_dependencies(${COMPILE_SHADER_TARGET} DirectXCompilerUnpack-build)

        set_target_properties(${COMPILE_SHADER_TARGET}
            PROPERTIES
            FOLDER "Build/${FOR_TARGET}/Shaders"
        )

        list(APPEND _OUT_COMPILED_SHADER_BINARIES ${SHADER_OBJ_PATH})
        list(APPEND _OUT_COMPILE_SHADER_TARGETS ${COMPILE_SHADER_TARGET})
    endforeach()

    set(${OUT_COMPILED_SHADER_BINARIES} ${_OUT_COMPILED_SHADER_BINARIES} PARENT_SCOPE)
    set(${OUT_COMPILE_SHADER_TARGETS} ${_OUT_COMPILE_SHADER_TARGETS} PARENT_SCOPE)
endfunction()

function(add_methane_shaders_source)
    set(ARGS_OPTIONS )
    set(ARGS_SINGLE_VALUE TARGET SOURCE VERSION)
    set(ARGS_MULTI_VALUE TYPES)
    list(APPEND ARGS_REQUIRED ${ARGS_SINGLE_VALUE})
    list(APPEND ARGS_REQUIRED ${ARGS_MULTI_VALUE})

    cmake_parse_arguments(SHADERS "${ARGS_OPTIONS}" "${ARGS_SINGLE_VALUE}" "${ARGS_MULTI_VALUE}" ${ARGN})
    send_cmake_parse_errors("add_methane_shaders_source" "SHADERS"
                            "${SHADERS_KEYWORDS_MISSING_VALUES}" "${SHADERS_UNPARSED_ARGUMENTS}" "${ARGS_REQUIRED}")

    set(SHADERS_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SHADERS_SOURCE}")
    set_property(TARGET ${SHADERS_TARGET} APPEND PROPERTY SHADER_SOURCES ${SHADERS_SOURCE_PATH})
    target_sources(${SHADERS_TARGET} PRIVATE ${SHADERS_SOURCE_PATH})

    # Disable automatic HLSL shaders compilation in Visual Studio, since it's compiled by custom target
    set_source_files_properties(${SHADERS_SOURCE_PATH}
        PROPERTIES
            VS_TOOL_OVERRIDE "None"
    )

    if (METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX OR
        METHANE_GFX_API EQUAL METHANE_GFX_VULKAN)

        compile_hlsl_shaders(${SHADERS_TARGET} "${SHADERS_SOURCE_PATH}" "${SHADERS_VERSION}" "${SHADERS_TYPES}" COMPILED_SHADER_BINARIES COMPILE_SHADER_TARGETS)
        set_property(TARGET ${SHADERS_TARGET} APPEND PROPERTY COMPILED_SHADER_BINARIES ${COMPILED_SHADER_BINARIES})
        set_property(TARGET ${SHADERS_TARGET} APPEND PROPERTY COMPILE_SHADER_TARGETS ${COMPILE_SHADER_TARGETS})

    elseif(METHANE_GFX_API EQUAL METHANE_GFX_METAL)

        set(SHADERS_METAL) # init with empty list
        get_metal_library(${SHADERS_TARGET} ${SHADERS_SOURCE_PATH} METAL_LIBRARY)
        generate_metal_shaders_from_hlsl(${SHADERS_TARGET} "${SHADERS_SOURCE_PATH}" "${SHADERS_VERSION}" "${SHADERS_TYPES}" SHADERS_METAL GENERATE_METAL_TARGETS)
        compile_metal_shaders_to_library(${SHADERS_TARGET} "macosx" "${SHADERS_METAL}" "${METAL_LIBRARY}")
        set_property(TARGET ${SHADERS_TARGET} APPEND PROPERTY METAL_SOURCES ${SHADERS_METAL})
        set_property(TARGET ${SHADERS_TARGET} APPEND PROPERTY METAL_LIBRARIES ${METAL_LIBRARY})
        set_property(TARGET ${SHADERS_TARGET} APPEND PROPERTY GENERATE_METAL_TARGETS ${GENERATE_METAL_TARGETS})

    endif()

endfunction()

function(add_methane_shaders_library TARGET)

    set(RESOURCE_NAMESPACE ${TARGET})
    get_generated_shader_extension(GENERATED_SHADER_EXT)

    get_target_property(TARGET_SHADER_SOURCES ${TARGET} SHADER_SOURCES)

    if (METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX OR
        METHANE_GFX_API EQUAL METHANE_GFX_VULKAN)

        target_sources(${TARGET} PRIVATE
            ${TARGET_SHADER_SOURCES}
        )

        get_target_property(TARGET_COMPILED_SHADER_BINARIES ${TARGET} COMPILED_SHADER_BINARIES)
        get_target_property(TARGET_COMPILE_SHADER_TARGETS ${TARGET} COMPILE_SHADER_TARGETS)

        get_target_shaders_dir(${TARGET} TARGET_SHADERS_DIR)

        shorten_target_name(${TARGET}_Shaders SHADER_RESOURCES_TARGET)
        cmrc_add_resource_library(${SHADER_RESOURCES_TARGET}
            ALIAS Methane::Resources::Shaders
            WHENCE "${TARGET_SHADERS_DIR}"
            NAMESPACE ${RESOURCE_NAMESPACE}::Shaders
            ${TARGET_COMPILED_SHADER_BINARIES}
        )

        add_dependencies(${SHADER_RESOURCES_TARGET} ${TARGET_COMPILE_SHADER_TARGETS})

        target_link_libraries(${SHADER_RESOURCES_TARGET} PRIVATE
            MethaneBuildOptions
        )

        target_link_libraries(${TARGET} PRIVATE
            ${SHADER_RESOURCES_TARGET}
        )

        target_compile_definitions(${TARGET}
            PRIVATE
            SHADER_RESOURCES_NAMESPACE=${RESOURCE_NAMESPACE}::Shaders
        )

        set_target_properties(${SHADER_RESOURCES_TARGET}
            PROPERTIES
            FOLDER "Build/${TARGET}/Resources"
        )

    elseif(METHANE_GFX_API EQUAL METHANE_GFX_METAL)

        get_target_property(TARGET_METAL_SOURCES ${TARGET} METAL_SOURCES)
        get_target_property(TARGET_METAL_LIBRARIES ${TARGET} METAL_LIBRARIES)
        get_target_property(TARGET_GENERATE_METAL_TARGETS ${TARGET} GENERATE_METAL_TARGETS)

        target_sources(${TARGET} PRIVATE
            ${TARGET_SHADER_SOURCES}
            ${TARGET_METAL_SOURCES}
            ${TARGET_METAL_LIBRARIES}
        )

        # Set bundle location for metal library files
        set_source_files_properties(${TARGET_METAL_LIBRARIES}
            PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources"
        )

        set_source_files_properties(
            ${TARGET_METAL_SOURCES}
            ${TARGET_METAL_LIBRARIES}
            PROPERTIES
                GENERATED TRUE
        )

        source_group("Generated Shaders" FILES
            ${TARGET_METAL_SOURCES}
            ${TARGET_METAL_LIBRARIES}
        )

        # Add Metal libraries to the list of prerequisites to auto-copy them to the application Resources
        set_target_properties(${TARGET}
            PROPERTIES
                PREREQUISITE_RESOURCES "${TARGET_METAL_LIBRARIES}"
        )

    endif()

    source_group("Source Shaders" FILES ${TARGET_SHADER_SOURCES})

endfunction()