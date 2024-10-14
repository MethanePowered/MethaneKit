/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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

FILE: MeshBuffersBase.h
Mesh buffers base implementation class.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/IContext.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/BufferSet.h>
#include <Methane/Graphics/RHI/ProgramBindings.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>
#include <Methane/Graphics/UberMesh.hpp>

#include <vector>
#include <string>

namespace Methane::Graphics
{

namespace Rhi
{
class CommandQueue;
class RenderCommandList;
class ParallelRenderCommandList;
}

struct MeshBufferBindings
{
    Rhi::Buffer          uniforms_buffer;
    Rhi::ProgramBindings program_bindings;
};

struct InstancedMeshBufferBindings
{
    Rhi::Buffer                       uniforms_buffer;
    std::vector<Rhi::ProgramBindings> program_bindings_per_instance;
};

class MeshBuffersBase
{
public:
    using InstancedProgramBindings = std::vector<Rhi::ProgramBindings>;
    using ProgramBindingsIteratorType = InstancedProgramBindings::const_iterator;

    MeshBuffersBase(const Rhi::CommandQueue& render_cmd_queue, const Mesh& mesh_data,
                    std::string_view mesh_name, const Mesh::Subsets& mesh_subsets);

    virtual ~MeshBuffersBase() = default;

    [[nodiscard]] const Rhi::IContext&  GetContext() const noexcept        { return m_context; }
    [[nodiscard]] const std::string&    GetMeshName() const noexcept       { return m_mesh_name; }
    [[nodiscard]] Data::Size            GetSubsetsCount() const noexcept   { return static_cast<Data::Size>(m_mesh_subsets.size()); }
    [[nodiscard]] const Rhi::BufferSet& GetVertexBuffers() const noexcept  { return m_vertex_buffer_set; }
    [[nodiscard]] const Rhi::Buffer&    GetIndexBuffer() const noexcept    { return m_index_buffer; }

    Rhi::ResourceBarriers CreateBeginningResourceBarriers(const Rhi::Buffer* constants_buffer_ptr = nullptr) const;

    void Draw(const Rhi::RenderCommandList& cmd_list,
              const Rhi::ProgramBindings& program_bindings,
              uint32_t mesh_subset_index = 0U,
              uint32_t instance_count = 1U,
              uint32_t start_instance = 0U) const;

    void Draw(const Rhi::RenderCommandList& cmd_list,
              const InstancedProgramBindings& instance_program_bindings,
              Rhi::ProgramBindingsApplyBehaviorMask bindings_apply_behavior = Rhi::ProgramBindingsApplyBehaviorMask(~0U),
              uint32_t first_instance_index = 0U, bool retain_bindings_once = false, bool set_resource_barriers = true) const;

    void Draw(const Rhi::RenderCommandList& cmd_list,
              const ProgramBindingsIteratorType& instance_program_bindings_begin,
              const ProgramBindingsIteratorType& instance_program_bindings_end,
              Rhi::ProgramBindingsApplyBehaviorMask bindings_apply_behavior = Rhi::ProgramBindingsApplyBehaviorMask(~0U),
              uint32_t first_instance_index = 0U, bool retain_bindings_once = false, bool set_resource_barriers = true) const;

    void DrawParallel(const Rhi::ParallelRenderCommandList& parallel_cmd_list,
                      const InstancedProgramBindings& instance_program_bindings,
                      Rhi::ProgramBindingsApplyBehaviorMask bindings_apply_behavior = Rhi::ProgramBindingsApplyBehaviorMask(~0U),
                      bool retain_bindings_once = false, bool set_resource_barriers = true) const;

protected:
    [[nodiscard]]
    virtual Data::Index GetSubsetByInstanceIndex(Data::Index instance_index) const { return instance_index; }

private:
    const Rhi::IContext& m_context;
    const std::string    m_mesh_name;
    const Mesh::Subsets  m_mesh_subsets;
    Rhi::BufferSet       m_vertex_buffer_set;
    Rhi::Buffer          m_index_buffer;
};

} // namespace Methane::Graphics