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

FILE: MethaneModules.cmake
Library module configuration functions

*****************************************************************************]]

function(get_target_arch OUT_ARCH)
    if(APPLE)
        set(${OUT_ARCH} "" PARENT_SCOPE)
    elseif(ARMEABI_V7A)
        set(${OUT_ARCH} "arm" PARENT_SCOPE)
    elseif(ARM64_V8A)
        set(${OUT_ARCH} "arm64" PARENT_SCOPE)
    elseif(${CMAKE_SIZEOF_VOID_P} STREQUAL "4")
        set(${OUT_ARCH} "x86" PARENT_SCOPE)
    elseif(${CMAKE_SIZEOF_VOID_P} STREQUAL "8")
        set(${OUT_ARCH} "x64" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Unknown architecture")
    endif()
endfunction()

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

function(get_target_resources_dir FOR_TARGET RESOURCES_DIR)
    set(TARGET_DIR $<TARGET_FILE_DIR:${FOR_TARGET}>)
    if(APPLE)
        set(${RESOURCES_DIR} ${TARGET_DIR}/../Resources PARENT_SCOPE)
    else()
        set(${RESOURCES_DIR} ${TARGET_DIR} PARENT_SCOPE)
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
        list(APPEND ALL_PREREQUISITE_MODULES ${FROM_TARGET})
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

function(add_prerequisite_binaries TO_TARGET FROM_TARGETS INSTALL_DIR)
    foreach(FROM_TARGET ${FROM_TARGETS})
        get_property(TARGET_PREREQUISITE_BINARIES_IS_SET TARGET ${FROM_TARGET} PROPERTY PREREQUISITE_BINARIES SET)
        if (TARGET_PREREQUISITE_BINARIES_IS_SET)
            get_target_property(COPY_BINARIES ${FROM_TARGET} PREREQUISITE_BINARIES)
            list(APPEND COPY_ALL_BINARIES ${COPY_BINARIES})
        endif()
        get_property(TARGET_PREREQUISITE_RESOURCES_IS_SET TARGET ${FROM_TARGET} PROPERTY PREREQUISITE_RESOURCES SET)
        if (TARGET_PREREQUISITE_RESOURCES_IS_SET)
            get_target_property(COPY_RESOURCES ${FROM_TARGET} PREREQUISITE_RESOURCES)
            list(APPEND COPY_ALL_RESOURCES ${COPY_RESOURCES})
        endif()
    endforeach()
    if (COPY_ALL_BINARIES)
        add_custom_command(TARGET ${TO_TARGET} POST_BUILD
            COMMENT "Copying prerequisite binaries for application ${TO_TARGET}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${COPY_ALL_BINARIES} "$<TARGET_FILE_DIR:${TO_TARGET}>"
        )
        if (INSTALL_DIR)
            install(FILES ${COPY_ALL_BINARIES}
                DESTINATION ${INSTALL_DIR}
            )
        endif()
    endif()
    if (COPY_ALL_RESOURCES)
        get_target_resources_dir(${TO_TARGET} RESOURCES_DIR)
        add_custom_command(TARGET ${TO_TARGET} POST_BUILD
            COMMENT "Copying prerequisite resources for application ${TO_TARGET}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${COPY_ALL_RESOURCES} "${RESOURCES_DIR}"
        )
        if (INSTALL_DIR AND NOT APPLE)
            install(FILES ${COPY_ALL_RESOURCES}
                DESTINATION ${INSTALL_DIR}
            )
        endif()
    endif()
endfunction()