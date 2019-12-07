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

function(add_methane_embedded_textures TARGET EMBEDDED_TEXTURES_DIR EMBEDDED_TEXTURES)

    set(TEXTURE_RESOURCES_TARGET ${TARGET}_Textures)
    set(RESOURCE_NAMESPACE ${TARGET})

    cmrc_add_resource_library(${TEXTURE_RESOURCES_TARGET}
        ALIAS Methane::Resources::Textures
        WHENCE "${EMBEDDED_TEXTURES_DIR}"
        NAMESPACE ${RESOURCE_NAMESPACE}::Textures
        ${EMBEDDED_TEXTURES}
    )

    set_target_properties(${TEXTURE_RESOURCES_TARGET}
        PROPERTIES
        FOLDER "Build/${TARGET}/Resources"
    )

    target_link_libraries(${TARGET}
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