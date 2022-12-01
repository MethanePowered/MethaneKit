add_subdirectory(Mouse)
add_subdirectory(Keyboard)
add_subdirectory(Controllers)
add_subdirectory(ActionControllers)

# TODO: Unified Input target to be removed and replaced with fine-grained dependencies
add_library(MethanePlatformInput INTERFACE)
target_link_libraries(MethanePlatformInput
    INTERFACE
        MethanePlatformInputMouse
        MethanePlatformInputKeyboard
        MethanePlatformInputControllers
        MethanePlatformInputActionControllers
)
