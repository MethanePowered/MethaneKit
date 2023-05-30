# Methane Graphics RHI Interface

## Graphics Devices and Contexts

- [ISystem, IDevice](Include/Methane/Graphics/IDevice.h)
- [IContext](Include/Methane/Graphics/IContext.h)
- [IRenderContext](Include/Methane/Graphics/IRenderContext.h)

## Graphics Commands Execution

- [ICommandQueue](Include/Methane/Graphics/ICommandQueue.h)
- [ICommandList, ICommandListSet](Include/Methane/Graphics/ICommandList.h)
- [IRenderCommandList](Include/Methane/Graphics/IRenderCommandList.h)
- [IParallelRenderCommandList](Include/Methane/Graphics/IParallelRenderCommandList.h)
- [ITransferCommandList](Include/Methane/Graphics/ITransferCommandList.h) (work in progress)
- [IComputeCommandList](Include/Methane/Graphics/IComputeCommandList.h) (work in progress)
- [IFence](Include/Methane/Graphics/IFence.h)
- [ICommandKit](Include/Methane/Graphics/ICommandKit.h)

## Graphics Resources

- [IResource](Include/Methane/Graphics/IResource.h)
    - [ResourceView, SubResource](Include/Methane/Graphics/ResourceView.h)
    - [IResourceBarriers](Include/Methane/Graphics/IResourceBarriers.h)
- [IBuffer, IBufferSet](Include/Methane/Graphics/IBuffer.h)
- [ITexture](Include/Methane/Graphics/ITexture.h)
- [ISampler](Include/Methane/Graphics/ISampler.h)

## Graphics Pipeline

- [IShader](Include/Methane/Graphics/IShader.h)
- [IProgram](Include/Methane/Graphics/IProgram.h)
- [IProgramBindings](Include/Methane/Graphics/IProgramBindings.h)
- [IRenderState](Include/Methane/Graphics/IRenderState.h)
- [IRenderPattern, IRenderPass](Include/Methane/Graphics/IRenderPass.h)

## Other
- [IObject, IObjectRegistry](Include/Methane/Graphics/IObject.h)