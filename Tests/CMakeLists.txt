list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

add_subdirectory(CatchHelpers)
add_subdirectory(Data)
add_subdirectory(Platform)
add_subdirectory(Graphics)
#add_subdirectory(UserInterface) # Temporarily disabled until RHI Null API is implemented for tests
add_subdirectory(Coverage)