include(CTest)
include(ParseAndAddCatchTests)

set(TARGET MethanePlatformInputTest)

add_executable(${TARGET}
    Main.cpp
    KeyboardTest.cpp
    MouseTest.cpp
)

target_link_libraries(${TARGET}
    MethanePlatformInput
    Catch
)

if (METHANE_RUN_TESTS_DURING_BUILD)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CTEST_EXE} -C $<CONFIGURATION> --output-on-failure
    )
endif()

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Tests
)

install(TARGETS ${TARGET}
    RUNTIME
        DESTINATION Tests
        COMPONENT Test
)

ParseAndAddCatchTests(${TARGET})