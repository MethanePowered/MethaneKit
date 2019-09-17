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

function(get_target_shaders_dir TARGET_SHADERS_DIR)
    set(${TARGET_SHADERS_DIR} "${CMAKE_CURRENT_BINARY_DIR}/Shaders" PARENT_SCOPE)
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

function(get_metal_library FOR_TARGET SHADERS_HLSL METAL_LIBRARY)
    get_target_shaders_dir(TARGET_SHADERS_DIR)
    get_shaders_name(${SHADERS_HLSL} SHADERS_NAME)
    set(${METAL_LIBRARY} "${TARGET_SHADERS_DIR}/${SHADERS_NAME}.metallib" PARENT_SCOPE)
endfunction()

function(get_generated_shaders FOR_TARGET SHADERS_CONFIG SHADER_EXT SHADERS_GENERATED)
    get_target_shaders_dir(TARGET_SHADERS_DIR)
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

function(generate_metal_shaders_from_hlsl FOR_TARGET SHADERS_HLSL OUT_SHADERS_METAL)
    get_platform_dir()
    get_target_shaders_dir(TARGET_SHADERS_DIR)
    get_shaders_name(${SHADERS_HLSL} SHADERS_NAME)
    get_shaders_config(${SHADERS_HLSL} SHADERS_CONFIG)

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

        list(APPEND SHADERS_METAL ${SHADER_METAL_PATH})
    endforeach()

    set(${OUT_SHADERS_METAL} ${SHADERS_METAL} PARENT_SCOPE)
endfunction()

function(compile_metal_shaders_to_library FOR_TARGET SDK METAL_SHADERS METAL_LIBRARY)
    get_target_shaders_dir(TARGET_SHADERS_DIR)

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
        BYPRODUCTS "${METAL_LIBRARY}"
        DEPENDS "${_SHADERS_AIR_FILES}"
        COMMAND xcrun -sdk ${SDK} metallib ${_SHADERS_AIR_FILES} -o "${METAL_LIBRARY}"
    )

    set_target_properties(${METAL_LIB_TARGET}
        PROPERTIES
        FOLDER Build
    )

    add_dependencies(${METAL_LIB_TARGET} ${_SHADER_COMPILE_TARGETS})
    add_dependencies(${FOR_TARGET} ${METAL_LIB_TARGET})
endfunction()

function(compile_hlsl_shaders FOR_TARGET SHADERS_HLSL PROFILE_VER OUT_COMPILED_SHADER_BINS)
    
    get_platform_dir()
    get_target_shaders_dir(TARGET_SHADERS_DIR)
    get_shaders_name(${SHADERS_HLSL} SHADERS_NAME)
    get_shaders_config(${SHADERS_HLSL} SHADERS_CONFIG)

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

        set(SHADER_OBJ_FILE "${SHADERS_NAME}_${NEW_ENTRY_POINT}.obj")
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

function(add_methane_application TARGET APP_NAME SOURCES RESOURCES_DIR INSTALL_DIR)
    set(BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")

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
            ${RESOURCE_FILE}
        )

        install(TARGETS ${TARGET}
            CONFIGURATIONS Release RelWithDebInfo
            RUNTIME
                DESTINATION ${INSTALL_DIR}
                COMPONENT Runtime
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

        add_executable(${TARGET} MACOSX_BUNDLE
            ${SOURCES}
            ${ICON_FILE_PATH}
        )

        install(TARGETS ${TARGET}
            BUNDLE
                DESTINATION ${INSTALL_DIR}
                COMPONENT Runtime
        )

        # Set bundle location of the icon and metal library files
        set_source_files_properties(${ICON_FILE_PATH}
            PROPERTIES MACOSX_PACKAGE_LOCATION
            "Resources"
        )

        set_target_properties(${TARGET}
            PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${PLIST_FILE_PATH}
        )

        set(BINARY_DIR ${BINARY_DIR}/${TARGET}.app/Contents/Resources)

        source_group("Resources" FILES ${PLIST_FILE_PATH} ${ICON_FILE_PATH})

    endif()

    source_group("Source Files" FILES ${SOURCES})
    source_group("Source Shaders" FILES ${SHADERS_HLSL} ${SHADERS_CONFIG})

    target_link_libraries(${TARGET}
        MethaneKit
    )

    get_target_property(METHANE_PREREQUISITE_MODULES MethaneKit PREREQUISITE_MODULES)
    add_prerequisite_binaries(${TARGET} "${METHANE_PREREQUISITE_MODULES}")

    target_include_directories(${TARGET}
        PUBLIC
            .
    )
endfunction()

function(add_methane_embedded_textures TARGET RESOURCE_NAMESPACE EMBEDDED_TEXTURES_DIR EMBEDDED_TEXTURES)

    set(TEXTURE_RESOURCES_TARGET ${TARGET}_Textures)

    cmrc_add_resource_library(${TEXTURE_RESOURCES_TARGET}
        ALIAS Methane::Resources::Textures
        WHENCE "${EMBEDDED_TEXTURES_DIR}"
        NAMESPACE ${RESOURCE_NAMESPACE}::Textures
        ${EMBEDDED_TEXTURES}
    )

    set_target_properties(${TEXTURE_RESOURCES_TARGET}
        PROPERTIES
        FOLDER Build
    )

    target_link_libraries(${TARGET}
        MethaneKit
        ${TEXTURE_RESOURCES_TARGET}
    )

    target_compile_definitions(${TARGET}
        PRIVATE
        TEXTURE_RESOURCE_NAMESPACE=${RESOURCE_NAMESPACE}::Textures
    )

endfunction()

function(add_methane_copy_textures TARGET COPY_TEXTURES)

    add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMENT "Copying textures for application " ${TARGET}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${BINARY_DIR}/Textures"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${COPY_TEXTURES} "${BINARY_DIR}/Textures"
        )

endfunction()

function(add_methane_shaders TARGET RESOURCE_NAMESPACE HLSL_SOURCES)

    if (WIN32)

        foreach(SHADERS_HLSL ${HLSL_SOURCES})
            get_shaders_config(${SHADERS_HLSL} SHADERS_CONFIG)
            get_generated_shaders(${TARGET} "${SHADERS_CONFIG}" "obj" SHADERS_OBJ)
            list(APPEND SHADERS_OBJ_FILES ${SHADERS_OBJ})
            list(APPEND CONFIG_SOURCES ${SHADERS_CONFIG})
        endforeach()
        
        get_target_shaders_dir(TARGET_SHADERS_DIR)

        set(SHADER_RESOURCES_TARGET ${TARGET}_Shaders)
        cmrc_add_resource_library(${SHADER_RESOURCES_TARGET}
            ALIAS Methane::Resources::Shaders
            WHENCE "${TARGET_SHADERS_DIR}"
            NAMESPACE ${RESOURCE_NAMESPACE}::Shaders
            ${SHADERS_OBJ_FILES}
        )

        foreach(SHADERS_HLSL ${HLSL_SOURCES})
            compile_hlsl_shaders(${TARGET} ${SHADERS_HLSL} "5_1" OUT_COMPILED_SHADER_BINS)
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
            FOLDER Build
        )

        target_compile_definitions(${TARGET}
            PRIVATE
            SHADER_RESOURCE_NAMESPACE=${RESOURCE_NAMESPACE}::Shaders
        )

        # Disable default manifest generation with linker, since manually written manifest is added to resources
        set_target_properties(${TARGET}
            PROPERTIES
            LINK_FLAGS "/MANIFEST:NO /ENTRY:mainCRTStartup"
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
            ${METAL_LIBRARY}
            PROPERTIES MACOSX_PACKAGE_LOCATION
            "Resources"
        )

        set_source_files_properties(
            ${GENERATED_SOURCES}
            ${METAL_LIBRARY}
            PROPERTIES GENERATED
            TRUE
        )

        source_group("Generated Shaders" FILES
            ${GENERATED_SOURCES}
            ${METAL_LIBRARY}
        )

        # Add Metal libraries to the list of prerequisites to auto-copy them to the application Resources
        set_target_properties(${TARGET}
            PROPERTIES PREREQUISITE_RESOURCES
            "${METAL_LIBRARIES}"
        )

        # Generate Metal shaders from HLSL sources with SPIRV toolset and compile to Metal Library
        foreach(SHADERS_HLSL ${HLSL_SOURCES})
            get_metal_library(${TARGET} ${SHADERS_HLSL} METAL_LIBRARY)
            generate_metal_shaders_from_hlsl(${TARGET} ${SHADERS_HLSL} SHADERS_METAL)
            compile_metal_shaders_to_library(${TARGET} "macosx" "${SHADERS_METAL}" "${METAL_LIBRARY}")
            list(APPEND SHADERS_GENERATED ${SHADERS_METAL})
        endforeach()

    endif()

    source_group("Source Shaders" FILES
        ${HLSL_SOURCES}
        ${CONFIG_SOURCES}
    )

endfunction()