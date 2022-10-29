# Methane Graphics Core

## Public Interfaces

This module implements a set of [public object-oriented interfaces](Include/Methane/Graphics),
which make modern graphics programming easy and convenient in a platform and API independent way:
![Graphics Core Interfaces](../../../Docs/Diagrams/MethaneKit_GraphicsCore_Interfaces.svg)

### Graphics Devices and Contexts

- [ISystem, IDevice](Include/Methane/Graphics/IDevice.h)
- [IContext](Include/Methane/Graphics/IContext.h)
- [IRenderContext](Include/Methane/Graphics/IRenderContext.h)

### Graphics Commands Execution

- [CommandQueue](Include/Methane/Graphics/CommandQueue.h)
- [CommandList, CommandListSet](Include/Methane/Graphics/CommandList.h)
- [RenderCommandList](Include/Methane/Graphics/RenderCommandList.h)
- [ParallelRenderCommandList](Include/Methane/Graphics/ParallelRenderCommandList.h)
- [TransferCommandList](Include/Methane/Graphics/TransferCommandList.h) (work in progress)
- [IFence](Include/Methane/Graphics/IFence.h)
- [CommandKit](Include/Methane/Graphics/CommandKit.h)

### Graphics Resources

- [IResource](Include/Methane/Graphics/IResource.h)
  - [ResourceView, SubResource](Include/Methane/Graphics/ResourceView.h)
  - [IResourceBarriers](Include/Methane/Graphics/IResourceBarriers.h)
- [IBuffer, IBufferSet](Include/Methane/Graphics/IBuffer.h)
- [ITexture](Include/Methane/Graphics/ITexture.h)
- [ISampler](Include/Methane/Graphics/ISampler.h)

### Graphics Pipeline

- [IShader](Include/Methane/Graphics/IShader.h)
- [IProgram](Include/Methane/Graphics/IProgram.h)
- [IProgramBindings](Include/Methane/Graphics/IProgramBindings.h)
- [IRenderState](Include/Methane/Graphics/IRenderState.h)
- [RenderPattern, RenderPass](Include/Methane/Graphics/RenderPass.h)

### Other
- [IObject, IObjectRegistry](Include/Methane/Graphics/IObject.h)

## Native Graphics API implementations

### DirectX 12

[DirectX12 implementation](Sources/Methane/Graphics/DirectX12) - full Methane Core API support.

### Metal

[Metal implementation](Sources/Methane/Graphics/Metal) - full Methane Core API support.

### Vulkan

[Vulkan implementation](Sources/Methane/Graphics/Vulkan) - **work in progress**:
basic functionality is supported required to render triangle or cube with vertex or index buffers,
but program bindings, textures, samplers, private resources upload are not supported yet.