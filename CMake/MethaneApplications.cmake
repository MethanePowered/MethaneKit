#[[****************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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

FILE: MethaneApplications.cmake
Creates and configures cross-platform graphics application cmake build target.

*****************************************************************************]]

include(MethaneUtils)
include(MethaneModules)
include(MethaneResources)

function(add_methane_application)
    set(ARGS_OPTIONS )
    set(ARGS_SINGLE_VALUE TARGET INSTALL_DIR NAME DESCRIPTION COPYRIGHT VERSION BUILD_NUMBER)
    set(ARGS_MULTI_VALUE SOURCES)
    set(ARGS_REQUIRED TARGET INSTALL_DIR NAME DESCRIPTION)
    list(APPEND ARGS_REQUIRED ${ARGS_MULTI_VALUE})

    cmake_parse_arguments(APP "${ARGS_OPTIONS}" "${ARGS_SINGLE_VALUE}" "${ARGS_MULTI_VALUE}" ${ARGN})
    send_cmake_parse_errors("add_methane_application" "APP" "${APP_KEYWORDS_MISSING_VALUES}" "${APP_UNPARSED_ARGUMENTS}" "${ARGS_REQUIRED}")

    set(BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")
    set(METHANE_APP_NAME ${APP_NAME})

    if (DEFINED APP_DESCRIPTION)
        set(METHANE_APP_DESCRIPTION ${APP_DESCRIPTION})
    else()
        set(METHANE_APP_DESCRIPTION ${PROJECT_DESCRIPTION})
    endif()

    if (DEFINED APP_COPYRIGHT)
        set(METHANE_APP_COPYRIGHT ${APP_COPYRIGHT})
    else()
        if (DEFINED METHANE_COPYRIGHT)
            set(METHANE_APP_COPYRIGHT ${METHANE_COPYRIGHT})
        else()
            set(METHANE_APP_COPYRIGHT ${PROJECT_HOMEPAGE_URL})
        endif()
    endif()

    if (DEFINED APP_VERSION)
        set(METHANE_APP_SHORT_VERSION "${APP_VERSION}")
    else()
        set(METHANE_APP_SHORT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    endif()

    if (NOT DEFINED APP_BUILD_NUMBER)
        set(APP_BUILD_NUMBER ${PROJECT_VERSION_TWEAK})
    endif()

    set(METHANE_APP_LONG_VERSION "${METHANE_APP_SHORT_VERSION}.${PROJECT_VERSION_PATCH}.${APP_BUILD_NUMBER}"

    if (WIN32)

        # Configure Resource and Manifest files
        set(METHANE_APP_MANIFEST_FILE_PATH ${CMAKE_CURRENT_BINARY_DIR}/${APP_TARGET}/App.manifest)
        set(METHANE_APP_RESOURCE_FILE_PATH ${CMAKE_CURRENT_BINARY_DIR}/${APP_TARGET}/Resource.rc)
        set(METHANE_APP_EXECUTABLE ${APP_TARGET}.exe)
        set(METHANE_APP_ICON_FILE_PATH ${METHANE_KIT_SOURCE_DIR}/Resources/Icons/Windows/Methane.ico)
        string(REPLACE "." "," METHANE_APP_SHORT_VERSION_CSV ${METHANE_APP_SHORT_VERSION})
        string(REPLACE "." "," METHANE_APP_LONG_VERSION_CSV ${METHANE_APP_LONG_VERSION})
        configure_file(${METHANE_KIT_SOURCE_DIR}/Resources/Configs/Windows/App.manifest.in ${METHANE_APP_MANIFEST_FILE_PATH})
        configure_file(${METHANE_KIT_SOURCE_DIR}/Resources/Configs/Windows/Resource.rc.in ${METHANE_APP_RESOURCE_FILE_PATH})

        set(MANIFEST_FILE ${CMAKE_CURRENT_BINARY_DIR}/${APP_TARGET}/Resource.rc)
        configure_file(${METHANE_KIT_SOURCE_DIR}/Resources/Configs/Windows/Resource.rc.in ${METHANE_APP_RESOURCE_FILE_PATH})

        add_executable(${APP_TARGET} WIN32
            ${APP_SOURCES}
            ${METHANE_APP_RESOURCE_FILE_PATH}
        )

        # Disable default manifest generation with linker, since manually written manifest is added to resources
        set_target_properties(${APP_TARGET}
            PROPERTIES
                LINK_FLAGS "/MANIFEST:NO /ENTRY:mainCRTStartup"
        )

        source_group("Resources" FILES ${METHANE_APP_RESOURCE_FILE_PATH} ${ICON_FILE_PATH})

    elseif(APPLE)

        if(APPLE_MACOS)
            set(CONFIG_DIR_NAME "MacOS")
        else()
            set(CONFIG_DIR_NAME "iOS")
            set(IOS_DEPLOYMENT_TARGET "${DEPLOYMENT_TARGET}")
        endif()

        set(ICON_FILE Methane.icns)
        set(ICON_FILE_PATH ${METHANE_KIT_SOURCE_DIR}/Resources/Icons/MacOS/${ICON_FILE})
        set(PLIST_FILE_PATH ${CMAKE_CURRENT_BINARY_DIR}/${APP_TARGET}/Info.plist)
        string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
        string(REPLACE "_" "-" PROJECT_NAME_URI ${PROJECT_NAME_LOWERCASE})

        # Configure bundle plist
        set(METHANE_APP_EXECUTABLE ${APP_TARGET})
        set(METHANE_APP_BUNDLE_VERSION ${APP_BUILD_NUMBER})
        set(METHANE_APP_BUNDLE_ICON ${ICON_FILE})
        set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.${PROJECT_NAME_URI}.${APP_TARGET}")
        set(MACOSX_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}") # Version applicable to MacOS only
        configure_file(${METHANE_KIT_SOURCE_DIR}/Resources/Configs/${CONFIG_DIR_NAME}/plist.in ${PLIST_FILE_PATH})

        add_executable(${APP_TARGET} MACOSX_BUNDLE
            ${APP_SOURCES}
            ${ICON_FILE_PATH}
        )

        # Set bundle location of the icon and metal library files
        set_source_files_properties(${ICON_FILE_PATH}
            PROPERTIES MACOSX_PACKAGE_LOCATION
            "Resources"
        )

        set_target_properties(${APP_TARGET}
            PROPERTIES
                MACOSX_BUNDLE_INFO_PLIST "${PLIST_FILE_PATH}"
                XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${MACOSX_BUNDLE_GUI_IDENTIFIER}"
                XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${APPLE_DEVELOPMENT_TEAM}"
                XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "${APPLE_CODE_SIGNING_FLAG}"
                XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "${APPLE_CODE_SIGNING_FLAG}"
        )

        if(APPLE_IOS)
            set_target_properties(${APP_TARGET}
                PROPERTIES
                    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone, iPad
            )
        endif()

        set(BINARY_DIR ${BINARY_DIR}/${APP_TARGET}.app/Contents/Resources)

        source_group("Resources" FILES ${PLIST_FILE_PATH} ${ICON_FILE_PATH})

    else() # Linux

        add_executable(${APP_TARGET}
            ${APP_SOURCES}
        )

        set(ICONS_DIR ${METHANE_KIT_SOURCE_DIR}/Resources/Icons/Linux)
        file(GLOB ICON_FILES "${ICONS_DIR}/Methane*.png")
        add_methane_embedded_icons(${APP_TARGET} "${ICONS_DIR}" "${ICON_FILES}")
    
    endif()

    source_group("Source Files" FILES ${APP_SOURCES})
    source_group("Source Shaders" FILES ${SHADERS_HLSL} ${SHADERS_CONFIG})

    target_link_libraries(${APP_TARGET}
        PRIVATE
            MethaneKit
            MethaneBuildOptions
            MethaneInstrumentation
            $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TracyClient>
    )

    target_precompile_headers(${APP_TARGET} REUSE_FROM MethaneKit)

    target_include_directories(${APP_TARGET}
        PRIVATE
            .
    )

    if (APPLE)
        install(TARGETS ${APP_TARGET}
            BUNDLE
                DESTINATION ${APP_INSTALL_DIR}
                COMPONENT Runtime
        )
    else()
        install(TARGETS ${APP_TARGET}
            RUNTIME
                DESTINATION ${APP_INSTALL_DIR}
                COMPONENT Runtime
        )
    endif()

    if (WIN32 AND MSVC)
        install(FILES $<TARGET_PDB_FILE:${APP_TARGET}>
            DESTINATION ${APP_INSTALL_DIR}
            OPTIONAL
        )
    endif()

    get_target_property(METHANE_PREREQUISITE_MODULES MethaneKit PREREQUISITE_MODULES)
    add_prerequisite_binaries(${APP_TARGET} "${METHANE_PREREQUISITE_MODULES}" ${APP_INSTALL_DIR})

endfunction()
