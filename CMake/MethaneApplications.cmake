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

function(add_methane_application TARGET SOURCES RESOURCES_DIR INSTALL_DIR APP_NAME DESCRIPTION COPYRIGHT VERSION BUILD_NUMBER)
    set(BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")

    set(METHANE_APP_NAME ${APP_NAME})
    set(METHANE_APP_DESCRIPTION ${DESCRIPTION})
    set(METHANE_APP_COPYRIGHT ${COPYRIGHT})
    set(METHANE_APP_SHORT_VERSION ${VERSION})
    get_full_file_version(${VERSION} ${BUILD_NUMBER} METHANE_APP_LONG_VERSION)

    if (WIN32)

        # Configure Resource and Manifest files
        set(METHANE_APP_MANIFEST_FILE_PATH ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/App.manifest)
        set(METHANE_APP_RESOURCE_FILE_PATH ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/Resource.rc)
        set(METHANE_APP_EXECUTABLE ${TARGET}.exe)
        set(METHANE_APP_ICON_FILE_PATH ${RESOURCES_DIR}/Icons/Windows/Methane.ico)
        string(REPLACE "." "," METHANE_APP_SHORT_VERSION_CSV ${METHANE_APP_SHORT_VERSION})
        string(REPLACE "." "," METHANE_APP_LONG_VERSION_CSV ${METHANE_APP_LONG_VERSION})
        configure_file(${RESOURCES_DIR}/Configs/Windows/App.manifest.in ${METHANE_APP_MANIFEST_FILE_PATH})
        configure_file(${RESOURCES_DIR}/Configs/Windows/Resource.rc.in ${METHANE_APP_RESOURCE_FILE_PATH})

        set(MANIFEST_FILE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/Resource.rc)
        configure_file(${RESOURCES_DIR}/Configs/Windows/Resource.rc.in ${METHANE_APP_RESOURCE_FILE_PATH})

        add_executable(${TARGET} WIN32
            ${SOURCES}
            ${METHANE_APP_RESOURCE_FILE_PATH}
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

        source_group("Resources" FILES ${METHANE_APP_RESOURCE_FILE_PATH} ${ICON_FILE_PATH})

    elseif(APPLE)

        set(ICON_FILE Methane.icns)
        set(ICON_FILE_PATH ${RESOURCES_DIR}/Icons/MacOS/${ICON_FILE})
        set(PLIST_FILE_PATH ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/Info.plist)

        # Configure bundle plist
        set(METHANE_APP_EXECUTABLE ${TARGET})
        set(METHANE_APP_BUNDLE_VERSION "${BUILD_NUMBER}")
        set(METHANE_APP_BUNDLE_ICON ${ICON_FILE})
        set(METHANE_APP_BUNDLE_GUI_IDENTIFIER "")
        set(MACOSX_DEPLOYMENT_TARGET "10.13")
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
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
            XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
        )

        set(BINARY_DIR ${BINARY_DIR}/${TARGET}.app/Contents/Resources)

        source_group("Resources" FILES ${PLIST_FILE_PATH} ${ICON_FILE_PATH})

    else() # Linux

        add_executable(${TARGET}
            ${SOURCES}
        )
    
    endif()

    source_group("Source Files" FILES ${SOURCES})
    source_group("Source Shaders" FILES ${SHADERS_HLSL} ${SHADERS_CONFIG})

    target_link_libraries(${TARGET}
        MethaneGraphicsKit
    )

    target_include_directories(${TARGET}
        PUBLIC
            .
    )

    get_target_property(METHANE_PREREQUISITE_MODULES MethaneGraphicsKit PREREQUISITE_MODULES)
    add_prerequisite_binaries(${TARGET} "${METHANE_PREREQUISITE_MODULES}" ${INSTALL_DIR})

endfunction()
