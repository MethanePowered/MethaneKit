# Methane Graphics Core

## Public Interfaces

This module implements a set of [public object-oriented interfaces](Include/Methane/Graphics),
which make modern graphics programming easy and convenient in a platform and API independent way:
![Graphics Core Interfaces](../../../Docs/Diagrams/MethaneKit_GraphicsCore_Interfaces.svg)

### Graphics Devices and Contexts

- [System, Device](Include/Methane/Graphics/Device.h)
- [Context](Include/Methane/Graphics/Context.h)
- [RenderContext](Include/Methane/Graphics/RenderContext.h)

### Graphics Commands Execution

- [CommandQueue](Include/Methane/Graphics/CommandQueue.h)
- [CommandList, CommandListSet](Include/Methane/Graphics/CommandList.h)
- [RenderCommandList](Include/Methane/Graphics/RenderCommandList.h)
- [ParallelRenderCommandList](Include/Methane/Graphics/ParallelRenderCommandList.h)
- [BlitCommandList](Include/Methane/Graphics/BlitCommandList.h) (work in progress)
- [Fence](Include/Methane/Graphics/Fence.h)
- [CommandKit](Include/Methane/Graphics/CommandKit.h)

### Graphics Resources

- [Resource](Include/Methane/Graphics/Resource.h)
  - [SubResource](Include/Methane/Graphics/SubResource.h)
  - [ResourceBarriers](Include/Methane/Graphics/ResourceBarriers.h)
- [Buffer, BufferSet](Include/Methane/Graphics/Buffer.h)
- [Texture](Include/Methane/Graphics/Texture.h)
- [Sampler](Include/Methane/Graphics/Sampler.h)

### Graphics Pipeline

- [Shader](Include/Methane/Graphics/Shader.h)
- [Program](Include/Methane/Graphics/Program.h)
- [ProgramBindings](Include/Methane/Graphics/ProgramBindings.h)
- [RenderState](Include/Methane/Graphics/RenderState.h)
- [RenderPass](Include/Methane/Graphics/RenderPass.h)

### Other
- [Object, Object::Registry](Include/Methane/Graphics/Object.h)