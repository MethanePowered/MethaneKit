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

FILE: MethaneApplications.cmake
Creates and configures cross-platform graphics application cmake build target.

*****************************************************************************]]

include(MethaneUtils)
include(MethaneModules)

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
