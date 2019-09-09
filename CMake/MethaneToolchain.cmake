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

FILE: Methane.cmake
Cmake auxillary functions to setup build of cross-platform graphics application

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

function(get_target_shader_paths FOR_TARGET)
    set(_TARGET_SHADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/Shaders/${FOR_TARGET}")
    set(TARGET_SHADERS_DIR "${_TARGET_SHADERS_DIR}" PARENT_SCOPE)
    set(SHADERS_METAL_LIB "${_TARGET_SHADERS_DIR}/default.metallib" PARENT_SCOPE)
endfunction()

function(get_shaders_config SHADERS_HLSL SHADERS_CONFIG)
    trim_spaces(${SHADERS_HLSL} SHADERS_HLSL)
    split_by_last_delimiter(${SHADERS_HLSL} "." SHADERS_PATH FILE_EXT)
    set(${SHADERS_CONFIG} "${SHADERS_PATH}.cfg" PARENT_SCOPE)
endfunction()

function(get_shaders_name SHADERS_FILE SHADERS_NAME)
    trim_spaces(${SHADERS_FILE} SHADERS_FILE)
    split_by_last_delimiter(${SHADERS_FILE} "." SHADERS_PATH FILE_EXT)
    string(REGEX MATCH "[^/\\]+$" FILE_NAME ${SHADERS_PATH})
    set(${SHADERS_NAME} ${FILE_NAME} PARENT_SCOPE)
endfunction()

function(get_generated_shaders FOR_TARGET SHADERS_CONFIG SHADER_EXT SHADERS_GENERATED)
    get_target_shader_paths(${FOR_TARGET})
    get_shaders_name(${SHADERS_CONFIG} SHADERS_NAME)

    file(STRINGS ${SHADERS_CONFIG} CONFIG_STRINGS)
    foreach(KEY_VALUE_STRING ${CONFIG_STRINGS})
        trim_spaces(${KEY_VALUE_STRING} KEY_VALUE_STRING)
        split_by_first_delimiter(${KEY_VALUE_STRING} "=" SHADER_TYPE ENTRY_POINT_WITH_DEFINIES)

        string(REGEX REPLACE ":| " "_" NEW_ENTRY_POINT ${ENTRY_POINT_WITH_DEFINIES})
        string(REPLACE "=" "" NEW_ENTRY_POINT "${NEW_ENTRY_POINT}")
        set(NEW_ENTRY_POINT "${SHADERS_NAME}_${NEW_ENTRY_POINT}")

        list(APPEND _SHADERS_GENERATED "${TARGET_SHADERS_DIR}/${NEW_ENTRY_POINT}.${SHADER_EXT}")
    endforeach()

    set(${SHADERS_GENERATED} ${_SHADERS_GENERATED} PARENT_SCOPE)
endfunction()

function(generate_metal_shaders_from_hlsl FOR_TARGET SHADERS_HLSL SHADERS_CONFIG)
    get_platform_dir()
    get_target_shader_paths(${FOR_TARGET})
    get_shaders_name(${SHADERS_CONFIG} SHADERS_NAME)

    set(SPIRV_BIN_DIR      "${CMAKE_SOURCE_DIR}/Externals/SPIRV/binaries/${PLATFORM_DIR}")
    set(SPIRV_GEN_EXE      "${SPIRV_BIN_DIR}/glslangValidator")
    set(SPIRV_CROSS_EXE    "${SPIRV_BIN_DIR}/spirv-cross")

    file(STRINGS ${SHADERS_CONFIG} CONFIG_STRINGS)
    foreach(KEY_VALUE_STRING ${CONFIG_STRINGS})
        trim_spaces(${KEY_VALUE_STRING} KEY_VALUE_STRING)
        split_by_first_delimiter(${KEY_VALUE_STRING} "=" SHADER_TYPE ENTRY_POINT_WITH_DEFINIES)
        split_by_first_delimiter(${ENTRY_POINT_WITH_DEFINIES} ":" OLD_ENTRY_POINT SHADER_DEFINITIONS)

        set(NEW_ENTRY_POINT "${SHADERS_NAME}_${OLD_ENTRY_POINT}")
        string(REPLACE " " ";" SHADER_DEFINITIONS "${SHADER_DEFINITIONS}")

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

        add_custom_target(${GENERATE_METAL_TARGET}
            COMMENT "Generating Metal shader source code from HLSL " ${NEW_ENTRY_POINT} " for application " ${TARGET}
            BYPRODUCTS "${SHADER_METAL_PATH}"
            DEPENDS ${SHADERS_HLSL} ${SHADERS_CONFIG}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGET_SHADERS_DIR}"
            COMMAND ${SPIRV_GEN_EXE} --hlsl-iomap -S ${SHADER_TYPE} -e ${OLD_ENTRY_POINT} ${SHADER_DEFINITION_ARGUMENTS} -o "${SHADER_SPIRV_PATH}" -V -D "${SHADERS_HLSL}"
            COMMAND ${SPIRV_CROSS_EXE} --msl --rename-entry-point ${OLD_ENTRY_POINT} ${NEW_ENTRY_POINT} ${SHADER_TYPE} --output "${SHADER_METAL_PATH}" "${SHADER_SPIRV_PATH}"
        )

        set_target_properties(${GENERATE_METAL_TARGET}
            PROPERTIES
            FOLDER Build
        )

        add_dependencies(${GENERATE_METAL_TARGET} SPIRV-build)
        add_dependencies(${FOR_TARGET} ${GENERATE_METAL_TARGET})
    endforeach()
endfunction()

function(compile_metal_shaders_to_library FOR_TARGET SDK METAL_SHADERS)
    get_target_shader_paths(${FOR_TARGET})

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
            COMMAND xcrun -sdk ${SDK} metal -gcodeview -c "${SHADER_METAL_PATH}" -o "${SHADER_AIR_PATH}"
        )

        set_target_properties(${SHADER_COMPILE_TARGET}
            PROPERTIES
            FOLDER Build
        )
        
        add_dependencies(${SHADER_COMPILE_TARGET} ${FOR_TARGET}_Generate_${SHADER_METAL_FILE})

        list(APPEND _SHADERS_AIR_FILES "${SHADER_AIR_PATH}")
        list(APPEND _SHADER_COMPILE_TARGETS ${SHADER_COMPILE_TARGET})
    endforeach()

    set(METAL_LIB_TARGET ${FOR_TARGET}_LinkMetalLibrary)
    add_custom_target(${METAL_LIB_TARGET}
        COMMENT "Linking default Metal library for application " ${TARGET}
        BYPRODUCTS "${_SHADERS_METAL_LIB}"
        DEPENDS "${_SHADERS_AIR_FILES}"
        COMMAND xcrun -sdk ${SDK} metallib ${_SHADERS_AIR_FILES} -o "${SHADERS_METAL_LIB}"
    )

    set_target_properties(${METAL_LIB_TARGET}
        PROPERTIES
        FOLDER Build
    )

    add_dependencies(${METAL_LIB_TARGET} ${_SHADER_COMPILE_TARGETS})
    add_dependencies(${FOR_TARGET} ${METAL_LIB_TARGET})
endfunction()

function(compile_hlsl_shaders FOR_TARGET SHADERS_HLSL SHADERS_CONFIG PROFILE_VER OUT_COMPILED_SHADER_BINS)
    
    get_platform_dir()
    get_target_shader_paths(${FOR_TARGET})

    set(SHADER_COMPILER_EXE "${WINDOWS_SDK_BIN_PATH}/fxc.exe")

    file(STRINGS ${SHADERS_CONFIG} CONFIG_STRINGS)
    foreach(KEY_VALUE_STRING ${CONFIG_STRINGS})
        trim_spaces(${KEY_VALUE_STRING} KEY_VALUE_STRING)
        split_by_first_delimiter(${KEY_VALUE_STRING} "=" SHADER_TYPE ENTRY_POINT_WITH_DEFINIES)
        split_by_first_delimiter(${ENTRY_POINT_WITH_DEFINIES} ":" ORIG_ENTRY_POINT SHADER_DEFINITIONS)

        set(NEW_ENTRY_POINT ${ORIG_ENTRY_POINT})
        string(REPLACE " " ";" SHADER_DEFINITIONS "${SHADER_DEFINITIONS}")

        foreach(SHADER_DEFINITION ${SHADER_DEFINITIONS})
            list(APPEND SHADER_DEFINITION_ARGUMENTS /D ${SHADER_DEFINITION})
            string(REPLACE "=" "" SHADER_DEFINITION_NAME ${SHADER_DEFINITION})
            set(NEW_ENTRY_POINT "${NEW_ENTRY_POINT}_${SHADER_DEFINITION_NAME}")
        endforeach()

        get_shader_profile(${SHADER_TYPE} ${PROFILE_VER} SHADER_PROFILE)

        set(SHADER_OBJ_FILE "${NEW_ENTRY_POINT}.obj")
        set(SHADER_OBJ_PATH "${TARGET_SHADERS_DIR}/${SHADER_OBJ_FILE}")

        list(APPEND _OUT_COMPILED_SHADER_BINS ${SHADER_OBJ_PATH})

        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(EXTRA_COMPILE_FLAGS /Od /Zi)
        endif()

        shorten_target_name(${FOR_TARGET}_HLSL_${NEW_ENTRY_POINT} COMPILE_SHADER_TARGET)
        add_custom_target(${COMPILE_SHADER_TARGET}
            COMMENT "Compiling HLSL shader from file " ${SHADERS_HLSL} " with profile " ${SHADER_PROFILE} " and macro-definition" ${SHADER_DEFINITIONS} "to OBJ file " ${SHADER_OBJ_FILE}
            BYPRODUCTS "${SHADER_OBJ_PATH}"
            DEPENDS ${SHADERS_HLSL} ${SHADERS_CONFIG}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGET_SHADERS_DIR}"
            COMMAND ${SHADER_COMPILER_EXE} /T ${SHADER_PROFILE} /E ${ORIG_ENTRY_POINT} /Fo ${SHADER_OBJ_PATH} ${EXTRA_COMPILE_FLAGS} ${SHADER_DEFINITION_ARGUMENTS} ${SHADERS_HLSL}
        )

        set_target_properties(${COMPILE_SHADER_TARGET}
            PROPERTIES
            FOLDER Build
        )

        add_dependencies(${FOR_TARGET}_Shaders ${COMPILE_SHADER_TARGET})
    endforeach()

    set(${OUT_COMPILED_SHADER_BINS} ${_OUT_COMPILED_SHADER_BINS} PARENT_SCOPE)
endfunction()

function(add_methane_application TARGET APP_NAME SOURCES HLSL_SOURCES RESOURCES_DIR EMBEDDED_TEXTURES_DIR EMBEDDED_TEXTURES COPY_TEXTURES INSTALL_DIR)
    set(BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")

    get_target_shader_paths(${TARGET})

    foreach(SHADERS_HLSL ${HLSL_SOURCES})
        get_shaders_config(${SHADERS_HLSL} SHADERS_CONFIG)
        list(APPEND CONFIG_SOURCES ${SHADERS_CONFIG})
    endforeach()

    if (WIN32)

        # Configure Resoruce file
        set(COMPONENT_NAME ${COMPONENT_OUTPUT_NAME}.exe)
        set(MANIEFEST_FILE_PATH ${RESOURCES_DIR}/Configs/Windows/App.manifest)
        set(ICON_FILE_PATH ${RESOURCES_DIR}/Icons/Windows/Methane.ico)
        set(RESOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/Resource.rc)
        set(FILE_DESCRIPTION ${APP_NAME})

        configure_file(${RESOURCES_DIR}/Configs/Windows/Resource.rc.in ${RESOURCE_FILE})

        add_executable(${TARGET} WIN32
            ${SOURCES}
            ${HLSL_SOURCES}
            ${CONFIG_SOURCES}
            ${RESOURCE_FILE}
        )

        install(TARGETS ${TARGET}
            CONFIGURATIONS Release RelWithDebInfo
            RUNTIME
                DESTINATION ${INSTALL_DIR}
                COMPONENT Runtime
        )

        get_generated_shaders(${TARGET} "${SHADERS_CONFIG}" "obj" SHADERS_OBJ)

        set(SHADER_RESOURCES_TARGET ${TARGET}_Shaders)
        cmrc_add_resource_library(${SHADER_RESOURCES_TARGET}
            ALIAS Methane::Resources::Shaders
            WHENCE "${TARGET_SHADERS_DIR}"
            NAMESPACE Shaders
            ${SHADERS_OBJ}
        )

        compile_hlsl_shaders(${TARGET} ${SHADERS_HLSL} ${SHADERS_CONFIG} "5_1" OUT_COMPILED_SHADER_BINS)

        # Disable automatic HLSL shaders compilation in Visual Studio, since it's compiled by custom target
        set_source_files_properties(${SHADERS_HLSL}
            PROPERTIES
            VS_TOOL_OVERRIDE "None"
        )

        set_target_properties(${SHADER_RESOURCES_TARGET}
            PROPERTIES
            FOLDER Build
        )

        target_compile_definitions(${TARGET}
            PRIVATE
            ENABLE_SHADER_RESOURCES
        )

        # Disable default manifest generation with linker, since manually written manifest is added to resources
        set_target_properties(${TARGET}
            PROPERTIES
                LINK_FLAGS "/MANIFEST:NO /ENTRY:mainCRTStartup"
        )

        source_group("Resources" FILES ${RESOURCE_FILE} ${ICON_FILE_PATH})

    elseif(APPLE)

        set(ICON_FILE Methane.icns)
        set(ICON_FILE_PATH ${RESOURCES_DIR}/Icons/MacOS/${ICON_FILE})
        set(PLIST_FILE_PATH ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)

        # Configure bundle plist
        set(MACOSX_BUNDLE_ICON_FILE ${ICON_FILE})
        set(MACOSX_BUNDLE_BUNDLE_NAME ${APP_NAME})
        configure_file(${RESOURCES_DIR}/Configs/MacOS/plist.in ${PLIST_FILE_PATH})
        
        # Get list of generated Metal shaders
        foreach(SHADERS_CONFIG ${CONFIG_SOURCES})
            get_generated_shaders(${TARGET} ${SHADERS_CONFIG} "metal" SHADERS_GENERATED)
            list(APPEND GENERATED_SOURCES ${SHADERS_GENERATED})
        endforeach()

        add_executable(${TARGET} MACOSX_BUNDLE
            ${SOURCES}
            ${HLSL_SOURCES}
            ${CONFIG_SOURCES}
            ${GENERATED_SOURCES}
            ${SHADERS_METAL_LIB}
            ${ICON_FILE_PATH}
        )

        install(TARGETS ${TARGET}
            BUNDLE
                DESTINATION ${INSTALL_DIR}
                COMPONENT Runtime
        )

        # Generate Metal shaders from HLSL sources with SPIRV toolset and compile to Metal Library
        foreach(SHADERS_HLSL ${HLSL_SOURCES})
            get_shaders_config(${SHADERS_HLSL} SHADERS_CONFIG)
            generate_metal_shaders_from_hlsl(${TARGET} ${SHADERS_HLSL} ${SHADERS_CONFIG})
        endforeach()

        compile_metal_shaders_to_library(${TARGET} "macosx" "${GENERATED_SOURCES}")
        
        # Set bundle location of the icon and metal library files
        set_source_files_properties(
            ${ICON_FILE_PATH}
            ${SHADERS_METAL_LIB}
            
            PROPERTIES MACOSX_PACKAGE_LOCATION
            "Resources"
        )

        set_source_files_properties(
            ${SHADERS_METAL_LIB}
            PROPERTIES
            GENERATED TRUE
        )
        
        set_target_properties(${TARGET}
            PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${PLIST_FILE_PATH}
        )

        set(BINARY_DIR ${BINARY_DIR}/${TARGET}.app/Contents/Resources)

        source_group("Generated Shaders" FILES ${SHADERS_GENERATED} ${SHADERS_METAL_LIB})
        source_group("Resources" FILES ${PLIST_FILE_PATH} ${ICON_FILE_PATH})

    endif()

    if (EMBEDDED_TEXTURES)
        set(TEXTURE_RESOURCES_TARGET ${TARGET}_Textures)
        cmrc_add_resource_library(${TEXTURE_RESOURCES_TARGET}
            ALIAS Methane::Resources::Textures
            WHENCE "${EMBEDDED_TEXTURES_DIR}"
            NAMESPACE Textures
            ${EMBEDDED_TEXTURES}
        )

        set_target_properties(${TEXTURE_RESOURCES_TARGET}
            PROPERTIES
            FOLDER Build
        )

        target_compile_definitions(${TARGET}
            PRIVATE
            ENABLE_TEXTURE_RESOURCES
        )
    endif()

    if (COPY_TEXTURES)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMENT "Copying textures for application " ${TARGET}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${BINARY_DIR}/Textures"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${COPY_TEXTURES} "${BINARY_DIR}/Textures"
        )
    endif()

    source_group("Source Files" FILES ${SOURCES})
    source_group("Source Shaders" FILES ${SHADERS_HLSL} ${SHADERS_CONFIG})

    target_link_libraries(${TARGET}
        MethaneKit
        ${SHADER_RESOURCES_TARGET}
        ${TEXTURE_RESOURCES_TARGET}
    )

    get_target_property(METHANE_PREREQUISITE_MODULES MethaneKit PREREQUISITE_MODULES)
    add_prerequisite_binaries(${TARGET} "${METHANE_PREREQUISITE_MODULES}")

    target_include_directories(${TARGET}
        PUBLIC
            .
    )
endfunction()
