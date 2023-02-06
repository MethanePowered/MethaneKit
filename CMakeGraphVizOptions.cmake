set(GRAPHVIZ_GRAPH_NAME "Methane Kit")
set(GRAPHVIZ_EXECUTABLES FALSE)
set(GRAPHVIZ_STATIC_LIBS TRUE)
set(GRAPHVIZ_SHARED_LIBS TRUE)
set(GRAPHVIZ_MODULE_LIBS TRUE)
set(GRAPHVIZ_INTERFACE_LIBS TRUE)
set(GRAPHVIZ_OBJECT_LIBS TRUE)
set(GRAPHVIZ_UNKNOWN_LIBS FALSE)
set(GRAPHVIZ_EXTERNAL_LIBS FALSE)
set(GRAPHVIZ_CUSTOM_TARGETS FALSE)
set(GRAPHVIZ_GENERATE_PER_TARGET TRUE)
set(GRAPHVIZ_GENERATE_DEPENDERS TRUE)
set(GRAPHVIZ_GRAPH_HEADER "    node [ fontsize = \"16\" ];")
set(GRAPHVIZ_IGNORE_TARGETS
    ^MethaneBuildOptions$
    ^MethaneInstrumentation$
    ^MethaneMathPrecompiledHeaders$
    ^MethaneTestsCatchHelpers$
    ^TracyInstrumentation$
    ^TracyClient$
    ^ittnotify$
    ^cmrc-base$
    ^magic_enum$
    ^CLI11_warnings$
    ^Catch2.*$
    ^nowide$
    ^fmt.*$
    ^default_settings.*$
    ^error_settings.*$
    ^.*_Shaders$
    ^.*_Textures$
    ^.*_Fonts$
    # MD5 hash of 32 chars length represent shortened target names (cmake regex does not support [...]{32} syntax)
    ^[a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9][a-f0-9]$
)