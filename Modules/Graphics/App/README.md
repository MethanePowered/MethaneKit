# Graphics App

Graphics application rendering infrastructure implemented on top of [application platform abstraction](../../Platform/App)
and [graphics RHI API](../RHI).

## Graphics Application Base Class

### [Graphics::IApp](Include/Methane/Graphics/IApp.h)

`Graphics::IApp` interface contains declaration of `Settings` structure and abstract methods
to get current graphics application settings and methods to modify individual setting values.
The following settings are available:

| IApp Setting             | Type               | Default Value | Cmd-Line Option | Description                                                                                            |
|--------------------------|--------------------|---------------|-----------------|--------------------------------------------------------------------------------------------------------|
| screen_pass_access       | IRenderPass::Access | None          |                 | Render pass access mask Graphics::IRenderPass::Access                                                   |
| animations_enabled       | bool               | true          | -a,--animations | Flag to enable or disable all animations                                                               |
| show_hud_in_window_title | bool               | true          |                 | Flag to display or hide graphics runtime parameters in window title                                    |
| default_device_index     | int32_t            | 0             | -d,--device     | Default GPU device used at startup: 0 - default h/w GPU, 1 - second h/w GPU, -1 - emulated WARP device |
| device_capabilities      | DeviceCaps         | Default       |                 | Device capabilities                                                                                    |

| DeviceCaps            | Type                 | Default Value | Description                                                 |
|-----------------------|----------------------|---------------|-------------------------------------------------------------|
| features              | Features             | All           | Required device features mask                               |
| present_to_window     | bool                 | true          | Flag of device is going to be used for presenting to window |
| render_queues_count   | uint32_t             | 1             | Count of render command queues used by application          |  
| transfer_queues_count | uint32_t             | 1             | Count of Transfer command queues used by application        |

### [Graphics::App](Include/Methane/Graphics/App.hpp)

Graphics application base template class `Graphics::App<Graphics::AppFrame>` is derived from [Platform::App](../../Platform/App)
extending it with common graphics application functionality implemented with [Graphics::RHI](../RHI) API:
- Parsing graphics app and context settings from command line arguments.
- Graphics render context initialization using provided settings.
- Initialization of the common graphics resources, such as frame-buffers and m_depth textures, final view state.
- Management of per-frame resources for [deferred rendering approach](https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-render)
such as frame buffer texture and final screen render pass stored in `Graphics::AppFrame` structure, which can be overridden
and extended with other frame-dependent resources in final applications.
- Window resize handling with re-creating frame buffer textures and final render passes for all frames.
- Updating graphics runtime parameters in HUD string in window header.
- Render function with implementation of common steps to be done before every render iterations, such as waiting for presenting previous frame.
- Adds [graphics application controllers](#graphics-application-controllers) to input state processor.
- Controlling animations pool, enabling and disabling all animations automatically on window resizing or be request.

`Graphics::AppSettings` structure aggregates 3 setting structures passed all together to the `Graphics::App` constructor:
- [Graphics::IApp::Settings](#graphicsiappincludemethanegraphicsapph) - graphics app settings described above
- [Platform::AppBase::Settings](../../Platform/App/README.md#platformappbaseincludemethaneplatformappbaseh) - platform app settings
- [Graphics::RenderContextSettings](../RHI/Interface/Include/Methane/Graphics/IRenderContext.h) - render context settings

Some parameters of the render context settings can be also changed with command line flags:

| Render Context Setting                               | Type     | Default Value | Cmd-Line Option             | Description           |
|------------------------------------------------------|----------|---------------|-----------------------------|-----------------------|
| vsync_enabled                                        | bool     | true          | -v,--vsync                  | Vertical synchronization |
| frame_buffers_count                                  | uint32_t | 3             | -b,--frame-buffers          | Frame buffers count in swap-chain |
| options_mask & Options::EmulatedRenderPassOnWindows  | bool     | false         | -e,--emulated-render-pass   | Render pass emulation on Windows |
| options_mask & Options::TransferWithDirectQueueOnWindows | bool     | false         | -q,--transfer-with-direct-queue | Transfer command lists and queues use DIRECT instead of COPY type in DX API |

## Graphics Application Controllers

### [Graphics::AppController](Include/Methane/Graphics/AppController.h)

Graphics **application** controller is derived from [Platform application controller](../../Platform/App/README.md#platformappcontrollerincludemethaneplatformappcontrollerh) and
extends it with the following actions:

| Application Action                  | Keyboard Shortcut   |
|-------------------------------------|---------------------|
| Switch Animations                   | LCtrl + P           |

### [Graphics::AppContextController](Include/Methane/Graphics/AppContextController.h)

Graphics application **context** controller implements the following actions:

| Render Context Action               | Keyboard Shortcut   |
|-------------------------------------|---------------------|
| Switch VSync                        | LCtrl + V           |
| Switch Device                       | LCtrl + X           |
| Add Frame Buffer to Swap-Chain      | LCtrl + Plus        |
| Remove Frame Buffer from Swap-Chain | LCtrl + Minus       |

### [Graphics::AppCameraController](Include/Methane/Graphics/AppCameraController.h)

Graphics application [Camera](../Camera) controller implements the following actions.

#### Mouse actions

| Camera Action                       | Mouse Buttons       |
|-------------------------------------|---------------------|
| Rotate Camera                       | Left Button         |
| Zoom Camera                         | Vertical Scroll     |
| Move Camera                         | Middle Button       |

#### Keyboard actions

| Camera Action                       | Keyboard Shortcut   |
|-------------------------------------|---------------------|
| Move Forward                        | W                   |
| Move Back                           | S                   |
| Move Left                           | A                   |
| Move Right                          | D                   |
| Move Up                             | Page-Up             |
| Move Down                           | Page-Down           |
| Roll Left                           | , / <               |
| Roll Right                          | . / >               |
| Yaw Left                            | Left Key            |
| Yaw Right                           | Right Key           |
| Pitch Up                            | Up Key              |
| Pitch Down                          | Down Key            |
| Zoom Out                            | -                   |
| Zoom In                             | +                   |
| Reset Orientation                   | LAlt + R            |
| Change Pivot Point                  | LAlt + P            |