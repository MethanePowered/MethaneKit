/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/DirectX/ComputeCommandList.cpp
DirectX 12 implementation of the transfer command list interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/ComputeCommandList.h>
#include <Methane/Graphics/DirectX/DescriptorManager.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/CommandQueue.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics::DirectX
{

ComputeCommandList::ComputeCommandList(Base::CommandQueue& cmd_queue)
    : CommandList<Base::ComputeCommandList>(D3D12_COMMAND_LIST_TYPE_COMPUTE, cmd_queue)
    , m_gpu_shader_resources_descriptor_heap(GetDirectCommandQueue().GetDirectContext().GetDirectDescriptorManager()
                                                .GetDefaultShaderVisibleDescriptorHeap(DescriptorHeap::Type::ShaderResources))
{ }

void ComputeCommandList::Reset(IDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    CommandList::Reset(debug_group_ptr);

    // Set GPU Visible Shader Resources descriptor heap to allow descriptor table resources binding
    ID3D12GraphicsCommandList& dx_command_list = GetNativeCommandListRef();
    ID3D12DescriptorHeap* d3d12_shader_resources_heap = m_gpu_shader_resources_descriptor_heap.GetNativeDescriptorHeap();
    dx_command_list.SetDescriptorHeaps(1U, &d3d12_shader_resources_heap);
}

void ComputeCommandList::Dispatch(const Rhi::ThreadGroupsCount& thread_groups_count)
{
    META_FUNCTION_TASK();
    ID3D12GraphicsCommandList& dx_command_list = GetNativeCommandListRef();
    dx_command_list.Dispatch(thread_groups_count.GetWidth(), thread_groups_count.GetHeight(), thread_groups_count.GetDepth());
}

} // namespace Methane::Graphics::DirectX
