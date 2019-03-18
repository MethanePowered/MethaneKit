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

include(CMakeRC)

function(get_platform_dir)
    if (WIN32)
        set(PLATFORM_DIR Windows PARENT_SCOPE)
        set(CPP_EXT cpp PARENT_SCOPE)
    elseif(APPLE)
        set(PLATFORM_DIR MacOS PARENT_SCOPE)
        set(CPP_EXT mm PARENT_SCOPE)
    else()
        set(PLATFORM_DIR Linux PARENT_SCOPE)
        set(CPP_EXT cpp PARENT_SCOPE)
    endif()
endfunction()

function(get_graphics_dir)
    if (WIN32)
        set(GRAPHICS_DIR DirectX12 PARENT_SCOPE)
    elseif(APPLE)
        set(GRAPHICS_DIR Metal PARENT_SCOPE)
    else()
        set(GRAPHICS_DIR Vulkan PARENT_SCOPE)
    endif()
endfunction()

function(get_module_dirs NAMESPACE)
    get_platform_dir()
    get_graphics_dir()

    set(INCLUDE_DIR "Include/${NAMESPACE}")
    set(SOURCES_DIR "Sources/${NAMESPACE}")

    set(INCLUDE_PLATFORM_DIR ${INCLUDE_DIR}/${PLATFORM_DIR} PARENT_SCOPE)
    set(SOURCES_PLATFORM_DIR ${SOURCES_DIR}/${PLATFORM_DIR} PARENT_SCOPE)
    set(INCLUDE_GRAPHICS_DIR ${INCLUDE_DIR}/${GRAPHICS_DIR} PARENT_SCOPE)
    set(SOURCES_GRAPHICS_DIR ${SOURCES_DIR}/${GRAPHICS_DIR} PARENT_SCOPE)
    set(INCLUDE_DIR ${INCLUDE_DIR} PARENT_SCOPE)
    set(SOURCES_DIR ${SOURCES_DIR} PARENT_SCOPE)
    set(NAMESPACE_DIR ${NAMESPACE} PARENT_SCOPE)
    set(GRAPHICS_DIR ${GRAPHICS_DIR} PARENT_SCOPE)
    set(PLATFORM_DIR ${PLATFORM_DIR} PARENT_SCOPE)
    set(CPP_EXT ${CPP_EXT} PARENT_SCOPE)
endfunction()

function(add_prerequisite_modules TO_TARGET FROM_TARGETS)
    foreach(FROM_TARGET ${FROM_TARGETS})
        if(NOT TARGET ${FROM_TARGET})
            message(SEND_ERROR "Can not add prerequisite modules from target \"${FROM_TARGET}\": target does not exist.")
        endif()
        get_property(TARGET_PREREQUISITE_MODULES_IS_SET TARGET ${FROM_TARGET} PROPERTY PREREQUISITE_MODULES SET)
        if (TARGET_PREREQUISITE_MODULES_IS_SET)
            get_target_property(TARGET_PREREQUISITE_MODULES ${FROM_TARGET} PREREQUISITE_MODULES)
            list(APPEND ALL_PREREQUISITE_MODULES ${TARGET_PREREQUISITE_MODULES})
        endif()
    endforeach()
    set_target_properties(${TO_TARGET}
        PROPERTIES
        PREREQUISITE_MODULES "${ALL_PREREQUISITE_MODULES}"
    )
endfunction()

function(add_prerequisite_binaries TO_TARGET FROM_TARGETS)
    foreach(FROM_TARGET ${FROM_TARGETS})
        get_property(TARGET_PREREQUISITE_BINARIES_IS_SET TARGET ${FROM_TARGET} PROPERTY PREREQUISITE_BINARIES SET)
        if (TARGET_PREREQUISITE_BINARIES_IS_SET)
            get_target_property(COPY_BINARIES ${FROM_TARGET} PREREQUISITE_BINARIES)
            list(APPEND COPY_ALL_BINARIES ${COPY_BINARIES})
        endif()
    endforeach()
    if (COPY_ALL_BINARIES)
        add_custom_command(TARGET ${TO_TARGET} POST_BUILD
            COMMENT "Copying prerequisite binaries for application " ${TO_TARGET}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${COPY_ALL_BINARIES} "$<TARGET_FILE_DIR:${TO_TARGET}>")
    endif()
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

function(get_generated_shaders FOR_TARGET SHADERS_CONFIG SHADER_EXT SHADERS_GENERATED)
    get_target_shader_paths(${FOR_TARGET})
    file(STRINGS ${SHADERS_CONFIG} CONFIG_STRINGS)
    foreach(KEY_VALUE_STRING ${CONFIG_STRINGS})
        string(REGEX REPLACE "^[ ]+" "" KEY_VALUE_STRING ${KEY_VALUE_STRING})
        string(REGEX MATCH "^[^=]+" SHADER_TYPE ${KEY_VALUE_STRING})
        string(REPLACE "${SHADER_TYPE}=" "" ENTRY_POINT_WITH_DEFINIES ${KEY_VALUE_STRING})
        string(REGEX REPLACE ":| " "_" NEW_ENTRY_POINT ${ENTRY_POINT_WITH_DEFINIES})
        string(REPLACE "=" "" NEW_ENTRY_POINT "${NEW_ENTRY_POINT}")

        list(APPEND _SHADERS_GENERATED "${TARGET_SHADERS_DIR}/${NEW_ENTRY_POINT}.${SHADER_EXT}")
    endforeach()
    set(${SHADERS_GENERATED} ${_SHADERS_GENERATED} PARENT_SCOPE)
endfunction()

function(generate_metal_shaders_from_hlsl FOR_TARGET SHADERS_HLSL SHADERS_CONFIG)
    get_platform_dir()
    get_target_shader_paths(${FOR_TARGET})

    set(SPIRV_BIN_DIR      "${CMAKE_SOURCE_DIR}/Externals/SPIRV/binaries/${PLATFORM_DIR}")
    set(SPIRV_GEN_EXE      "${SPIRV_BIN_DIR}/glslangValidator")
    set(SPIRV_CROSS_EXE    "${SPIRV_BIN_DIR}/spirv-cross")

    file(STRINGS ${SHADERS_CONFIG} CONFIG_STRINGS)
    foreach(KEY_VALUE_STRING ${CONFIG_STRINGS})
        string(REGEX REPLACE "^[ ]+" "" KEY_VALUE_STRING ${KEY_VALUE_STRING})
        string(REGEX MATCH "^[^=]+" SHADER_TYPE ${KEY_VALUE_STRING})
        string(REPLACE "${SHADER_TYPE}=" "" ENTRY_POINT_WITH_DEFINIES ${KEY_VALUE_STRING})
        string(REGEX MATCH "^[^:]+" OLD_ENTRY_POINT ${ENTRY_POINT_WITH_DEFINIES})

        set(NEW_ENTRY_POINT ${OLD_ENTRY_POINT})
        string(REGEX REPLACE "^${OLD_ENTRY_POINT}[:]?" "" SHADER_DEFINITIONS ${ENTRY_POINT_WITH_DEFINIES})
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
        string(REGEX REPLACE "^[ ]+" "" KEY_VALUE_STRING ${KEY_VALUE_STRING})
        string(REGEX MATCH "^[^=]+" SHADER_TYPE ${KEY_VALUE_STRING})
        string(REPLACE "${SHADER_TYPE}=" "" ENTRY_POINT_WITH_DEFINIES ${KEY_VALUE_STRING})
        string(REGEX MATCH "^[^:]+" ORIG_ENTRY_POINT ${ENTRY_POINT_WITH_DEFINIES})

        set(NEW_ENTRY_POINT ${ORIG_ENTRY_POINT})
        string(REGEX REPLACE "^${ORIG_ENTRY_POINT}[:]?" "" SHADER_DEFINITIONS ${ENTRY_POINT_WITH_DEFINIES})
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

function(add_methane_application TARGET APP_NAME SOURCES SHADERS_HLSL SHADERS_CONFIG RESOURCES_DIR EMBEDDED_TEXTURES_DIR EMBEDDED_TEXTURES COPY_TEXTURES INSTALL_DIR)
    set(BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")

    get_target_shader_paths(${TARGET})

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
            ${SHADERS_HLSL}
            ${SHADERS_CONFIG}
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
        get_generated_shaders(${TARGET} ${SHADERS_CONFIG} "metal" SHADERS_GENERATED)

        add_executable(${TARGET} MACOSX_BUNDLE
            ${SOURCES}
            ${SHADERS_HLSL}
            ${SHADERS_CONFIG}
            ${SHADERS_GENERATED}
            ${SHADERS_METAL_LIB}
            ${ICON_FILE_PATH}
        )

        install(TARGETS ${TARGET}
            BUNDLE
                DESTINATION ${INSTALL_DIR}
                COMPONENT Runtime
        )

        # Generate Metal shaders from HLSL sources with SPIRV toolset and compile to Metal Library
        generate_metal_shaders_from_hlsl(${TARGET} ${SHADERS_HLSL} ${SHADERS_CONFIG})
        compile_metal_shaders_to_library(${TARGET} "macosx" "${SHADERS_GENERATED}")
        
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
