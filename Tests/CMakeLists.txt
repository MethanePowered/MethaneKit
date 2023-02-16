list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

enable_testing()

if (CMAKE_INSTALL_PREFIX AND EXISTS "${CMAKE_INSTALL_PREFIX}/bin/ctest")
    set(CTEST_EXE "${CMAKE_INSTALL_PREFIX}/bin/ctest")
else()
    set(CTEST_EXE "ctest")
endif()

add_subdirectory(CatchHelpers)
add_subdirectory(Data)
add_subdirectory(Platform)
add_subdirectory(Graphics)
add_subdirectory(UserInterface)
add_subdirectory(Coverage)