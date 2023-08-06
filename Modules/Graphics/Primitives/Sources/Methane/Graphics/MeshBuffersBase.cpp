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

FILE: MeshBuffersBase.cpp
Mesh buffers base implementation class.

******************************************************************************/

#include <Methane/Graphics/MeshBuffersBase.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderCommandList.h>
#include <Methane/Graphics/RHI/ParallelRenderCommandList.h>
#include <Methane/Graphics/TypeConverters.hpp>
#include <Methane/Instrumentation.h>

#include <taskflow/algorithm/for_each.hpp>
#include <fmt/format.h>

namespace Methane::Graphics
{

MeshBuffersBase::MeshBuffersBase(const Rhi::CommandQueue& render_cmd_queue, const Mesh& mesh_data,
                                 std::string_view mesh_name, const Mesh::Subsets& mesh_subsets)
    : m_context(render_cmd_queue.GetContext())
    , m_mesh_name(mesh_name)
    , m_mesh_subsets(!mesh_subsets.empty()
                    ? mesh_subsets
                    : Mesh::Subsets{
                        Mesh::Subset(mesh_data.GetType(),
                                     { 0, mesh_data.GetVertexCount() },
                                     { 0, mesh_data.GetIndexCount()  }, true )
                      })
{
    META_FUNCTION_TASK();

    Rhi::Buffer vertex_buffer(m_context,
        Rhi::BufferSettings::ForVertexBuffer(
            mesh_data.GetVertexDataSize(),
            mesh_data.GetVertexSize()));
    vertex_buffer.SetName(fmt::format("{} Vertex Buffer", mesh_name));
    vertex_buffer.SetData(render_cmd_queue, {
        mesh_data.GetVertexData(),
        mesh_data.GetVertexDataSize()
    });
    m_vertex_buffer_set = Rhi::BufferSet(Rhi::BufferType::Vertex, { vertex_buffer });

    m_index_buffer = Rhi::Buffer(m_context,
        Rhi::BufferSettings::ForIndexBuffer(
            mesh_data.GetIndexDataSize(),
            GetIndexFormat(mesh_data.GetIndex(0))));
    m_index_buffer.SetName(fmt::format("{} Index Buffer", mesh_name));
    m_index_buffer.SetData(render_cmd_queue, {
        reinterpret_cast<Data::ConstRawPtr>(mesh_data.GetIndices().data()), // NOSONAR
        mesh_data.GetIndexDataSize()
    });
}

Rhi::ResourceBarriers MeshBuffersBase::CreateBeginningResourceBarriers(const Rhi::Buffer* constants_buffer_ptr) const
{
    META_FUNCTION_TASK();
    Rhi::ResourceBarriers beginning_resource_barriers({
        { GetIndexBuffer().GetInterface(), GetIndexBuffer().GetState(), Rhi::ResourceState::IndexBuffer },
    });

    if (constants_buffer_ptr)
    {
        beginning_resource_barriers.AddStateTransition(constants_buffer_ptr->GetInterface(),
                                                       constants_buffer_ptr->GetState(),
                                                       Rhi::ResourceState::ConstantBuffer);
    }

    for (Data::Index vertex_buffer_index = 0U; vertex_buffer_index < m_vertex_buffer_set.GetCount(); ++vertex_buffer_index)
    {
        const Rhi::Buffer& vertex_buffer = m_vertex_buffer_set[vertex_buffer_index];
        beginning_resource_barriers.AddStateTransition(vertex_buffer.GetInterface(),
                                                       vertex_buffer.GetState(),
                                                       Rhi::ResourceState::VertexBuffer);
    }

    return beginning_resource_barriers;
}

void MeshBuffersBase::Draw(const Rhi::RenderCommandList& cmd_list,
                           const Rhi::ProgramBindings& program_bindings,
                           uint32_t mesh_subset_index,
                           uint32_t instance_count,
                           uint32_t start_instance) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS_DESCR(mesh_subset_index, m_mesh_subsets.size(), "can not draw mesh subset because its index is out of bounds");

    const Mesh::Subset& mesh_subset = m_mesh_subsets[mesh_subset_index];
    cmd_list.SetProgramBindings(program_bindings);
    cmd_list.SetVertexBuffers(GetVertexBuffers());
    cmd_list.SetIndexBuffer(GetIndexBuffer());
    cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle,
                         mesh_subset.indices.count, mesh_subset.indices.offset,
                         mesh_subset.indices_adjusted ? 0 : mesh_subset.vertices.offset,
                         instance_count, start_instance);
}

void MeshBuffersBase::Draw(const Rhi::RenderCommandList& cmd_list,
                           const std::vector<Rhi::ProgramBindings>& instance_program_bindings,
                           Rhi::ProgramBindingsApplyBehaviorMask bindings_apply_behavior,
                           uint32_t first_instance_index,
                           bool retain_bindings_once,
                           bool set_resource_barriers) const
{
    META_FUNCTION_TASK();
    Draw(cmd_list, instance_program_bindings.begin(), instance_program_bindings.end(),
         bindings_apply_behavior, first_instance_index, retain_bindings_once, set_resource_barriers);
}

void MeshBuffersBase::Draw(const Rhi::RenderCommandList& cmd_list,
                           const ProgramBindingsIteratorType& instance_program_bindings_begin,
                           const ProgramBindingsIteratorType& instance_program_bindings_end,
                           Rhi::ProgramBindingsApplyBehaviorMask bindings_apply_behavior,
                           uint32_t first_instance_index, bool retain_bindings_once, bool set_resource_barriers) const
{
    META_FUNCTION_TASK();
    cmd_list.SetVertexBuffers(GetVertexBuffers(), set_resource_barriers);
    cmd_list.SetIndexBuffer(GetIndexBuffer(), set_resource_barriers);

    for (ProgramBindingsIteratorType instance_program_bindings_it = instance_program_bindings_begin;
         instance_program_bindings_it != instance_program_bindings_end;
         ++instance_program_bindings_it)
    {
        const Rhi::ProgramBindings& program_bindings = *instance_program_bindings_it;
        META_CHECK_ARG_TRUE(program_bindings.IsInitialized());

        const uint32_t instance_index = first_instance_index + static_cast<uint32_t>(std::distance(instance_program_bindings_begin, instance_program_bindings_it));
        const uint32_t subset_index = GetSubsetByInstanceIndex(instance_index);

        META_CHECK_ARG_LESS(subset_index, m_mesh_subsets.size());
        const Mesh::Subset& mesh_subset = m_mesh_subsets[subset_index];

        Rhi::ProgramBindingsApplyBehaviorMask apply_behavior = bindings_apply_behavior;
        apply_behavior.SetBit(Rhi::ProgramBindingsApplyBehavior::RetainResources,
                              !retain_bindings_once || instance_program_bindings_it == instance_program_bindings_begin);

        cmd_list.SetProgramBindings(program_bindings, apply_behavior);
        cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle,
                             mesh_subset.indices.count, mesh_subset.indices.offset,
                             mesh_subset.indices_adjusted ? 0 : mesh_subset.vertices.offset,
                             1, 0);
    }
}

void MeshBuffersBase::DrawParallel(const Rhi::ParallelRenderCommandList& parallel_cmd_list,
                                   const std::vector<Rhi::ProgramBindings>& instance_program_bindings,
                                   Rhi::ProgramBindingsApplyBehaviorMask bindings_apply_behavior,
                                   bool retain_bindings_once, bool set_resource_barriers) const
{
    META_FUNCTION_TASK();
    const std::vector<Rhi::RenderCommandList>& render_cmd_lists = parallel_cmd_list.GetParallelCommandLists();
    const auto instances_count_per_command_list = static_cast<uint32_t>(Data::DivCeil(instance_program_bindings.size(), render_cmd_lists.size()));

    tf::Taskflow render_task_flow;
    render_task_flow.for_each_index(0U, static_cast<uint32_t>(render_cmd_lists.size()), 1U,
        [this, &render_cmd_lists, instances_count_per_command_list, &instance_program_bindings,
        bindings_apply_behavior, retain_bindings_once, set_resource_barriers](const uint32_t cmd_list_index)
        {
            const Rhi::RenderCommandList& render_cmd_list = render_cmd_lists[cmd_list_index];
            const uint32_t begin_instance_index = cmd_list_index * instances_count_per_command_list;
            const uint32_t end_instance_index = std::min(begin_instance_index + instances_count_per_command_list,
                                                         static_cast<uint32_t>(instance_program_bindings.size()));

            Draw(render_cmd_list,
                 instance_program_bindings.begin() + begin_instance_index,
                 instance_program_bindings.begin() + end_instance_index,
                 bindings_apply_behavior, begin_instance_index,
                 retain_bindings_once, set_resource_barriers);
        }
    );
    m_context.GetParallelExecutor().run(render_task_flow).get();
}

} // namespace Methane::Graphics
