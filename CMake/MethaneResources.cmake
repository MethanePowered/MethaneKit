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

FILE: MethaneResources.cmake
Functions to add textures to the Methane module/application resources.

*****************************************************************************]]

include(CMakeRC)

function(add_methane_embedded_fonts TARGET EMBEDDED_FONTS_DIR EMBEDDED_FONTS)

    set(FONT_RESOURCES_TARGET ${TARGET}_Fonts)
    set(FONT_RESOURCES_NAMESPACE ${TARGET}::Fonts)

    cmrc_add_resource_library(${FONT_RESOURCES_TARGET}
        ALIAS Methane::Resources::Fonts
        WHENCE "${EMBEDDED_FONTS_DIR}"
        NAMESPACE ${FONT_RESOURCES_NAMESPACE}
        ${EMBEDDED_FONTS}
    )

    set_target_properties(${FONT_RESOURCES_TARGET}
        PROPERTIES
        FOLDER "Build/${TARGET}/Resources"
    )

    target_link_libraries(${FONT_RESOURCES_TARGET} PRIVATE
        MethaneBuildOptions
    )

    target_link_libraries(${TARGET} PRIVATE
        ${FONT_RESOURCES_TARGET}
    )

    target_compile_definitions(${TARGET}
        PRIVATE
        FONT_RESOURCES_NAMESPACE=${FONT_RESOURCES_NAMESPACE}
    )

endfunction()

function(add_methane_embedded_textures TARGET EMBEDDED_TEXTURES_DIR EMBEDDED_TEXTURES)

    set(TEXTURE_RESOURCES_TARGET ${TARGET}_Textures)
    set(TEXTURE_RESOURCES_NAMESPACE ${TARGET}::Textures)

    cmrc_add_resource_library(${TEXTURE_RESOURCES_TARGET}
        ALIAS Methane::Resources::Textures
        WHENCE "${EMBEDDED_TEXTURES_DIR}"
        NAMESPACE ${TEXTURE_RESOURCES_NAMESPACE}
        ${EMBEDDED_TEXTURES}
    )

    set_target_properties(${TEXTURE_RESOURCES_TARGET}
        PROPERTIES
        FOLDER "Build/${TARGET}/Resources"
    )

    target_link_libraries(${TEXTURE_RESOURCES_TARGET} PRIVATE
        MethaneBuildOptions
    )

    target_link_libraries(${TARGET} PRIVATE
        ${TEXTURE_RESOURCES_TARGET}
    )

    target_compile_definitions(${TARGET}
        PRIVATE
        TEXTURE_RESOURCES_NAMESPACE=${TEXTURE_RESOURCES_NAMESPACE}
    )

endfunction()

function(add_methane_embedded_icons TARGET EMBEDDED_ICONS_DIR EMBEDDED_ICONS)

    set(ICON_RESOURCES_TARGET ${TARGET}_Icons)
    set(ICON_RESOURCES_NAMESPACE ${TARGET}::Icons)

    cmrc_add_resource_library(${ICON_RESOURCES_TARGET}
        ALIAS Methane::Resources::Icons
        WHENCE "${EMBEDDED_ICONS_DIR}"
        NAMESPACE ${ICON_RESOURCES_NAMESPACE}
        ${EMBEDDED_ICONS}
    )

    set_target_properties(${ICON_RESOURCES_TARGET}
        PROPERTIES
        FOLDER "Build/${TARGET}/Resources"
    )

    target_link_libraries(${ICON_RESOURCES_TARGET} PRIVATE
        MethaneBuildOptions
    )

    target_link_libraries(${TARGET} PRIVATE
        ${ICON_RESOURCES_TARGET}
    )

    target_compile_definitions(${TARGET}
        PRIVATE
        ICON_RESOURCES_NAMESPACE=${ICON_RESOURCES_NAMESPACE}
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