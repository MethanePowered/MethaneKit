/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: MeshBuffers.hpp
Mesh buffers with texture extension structure.

******************************************************************************/

#pragma once

#include "ImageLoader.h"

#include <Methane/Graphics/Buffer.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Graphics/Program.h>
#include <Methane/Graphics/CommandQueue.h>
#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/ParallelRenderCommandList.h>
#include <Methane/Graphics/UberMesh.hpp>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/TypeConverters.hpp>
#include <Methane/Data/AlignedAllocator.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <taskflow/taskflow.hpp>
#include <fmt/format.h>

#include <memory>
#include <string>

namespace Methane::Graphics
{

struct MeshBufferBindings
{
    Ptr<Buffer>          uniforms_buffer_ptr;
    Ptr<ProgramBindings> program_bindings_ptr;
};
    
struct InstancedMeshBufferBindings
{
    Ptr<Buffer>           uniforms_buffer_ptr;
    Ptrs<ProgramBindings> program_bindings_per_instance;
};

template<typename UniformsType>
class MeshBuffers
{
public:
    template<typename VType>
    MeshBuffers(CommandQueue& render_cmd_queue, const BaseMesh<VType>& mesh_data, const std::string& mesh_name, const Mesh::Subsets& mesh_subsets = Mesh::Subsets())
        : m_context(render_cmd_queue.GetContext())
        , m_mesh_name(mesh_name)
        , m_mesh_subsets(!mesh_subsets.empty() ? mesh_subsets
                                               : Mesh::Subsets{ Mesh::Subset(mesh_data.GetType(), { 0, mesh_data.GetVertexCount() },
                                                                                                  { 0, mesh_data.GetIndexCount()  }, true ) })
    {
        META_FUNCTION_TASK();
        SetInstanceCount(static_cast<Data::Size>(m_mesh_subsets.size()));

        Ptr<Buffer> vertex_buffer_ptr = Buffer::CreateVertexBuffer(render_cmd_queue.GetContext(),
                                                                   static_cast<Data::Size>(mesh_data.GetVertexDataSize()),
                                                                   static_cast<Data::Size>(mesh_data.GetVertexSize()));
        vertex_buffer_ptr->SetName(fmt::format("{} Vertex Buffer", mesh_name));
        vertex_buffer_ptr->SetData({
            {
                reinterpret_cast<Data::ConstRawPtr>(mesh_data.GetVertices().data()), // NOSONAR
                static_cast<Data::Size>(mesh_data.GetVertexDataSize())
            }
        }, render_cmd_queue);
        m_vertex_ptr = BufferSet::CreateVertexBuffers({ *vertex_buffer_ptr });

        m_index_ptr = Buffer::CreateIndexBuffer(render_cmd_queue.GetContext(), static_cast<Data::Size>(mesh_data.GetIndexDataSize()), GetIndexFormat(mesh_data.GetIndex(0)));
        m_index_ptr->SetName(fmt::format("{} Index Buffer", mesh_name));
        m_index_ptr->SetData({
            {
                reinterpret_cast<Data::ConstRawPtr>(mesh_data.GetIndices().data()), // NOSONAR
                static_cast<Data::Size>(mesh_data.GetIndexDataSize())
            }
        }, render_cmd_queue);
    }

    template<typename VType>
    MeshBuffers(CommandQueue& render_cmd_queue, const UberMesh<VType>& uber_mesh_data, const std::string& mesh_name)
        : MeshBuffers(render_cmd_queue, uber_mesh_data, mesh_name, uber_mesh_data.GetSubsets())
    {
        META_FUNCTION_TASK();
    }
    
    virtual ~MeshBuffers() = default;

    [[nodiscard]] const Context& GetContext() const noexcept { return m_context; }

    Ptr<Resource::Barriers> CreateBeginningResourceBarriers(Buffer* constants_buffer_ptr = nullptr)
    {
        META_FUNCTION_TASK();
        Ptr<Resource::Barriers> beginning_resource_barriers_ptr = Resource::Barriers::Create({
            { GetIndexBuffer(), GetIndexBuffer().GetState(), Resource::State::IndexBuffer },
        });

        if (constants_buffer_ptr)
        {
            beginning_resource_barriers_ptr->AddStateTransition(*constants_buffer_ptr, constants_buffer_ptr->GetState(), Resource::State::ConstantBuffer);
        }

        const BufferSet& vertex_buffer_set = GetVertexBuffers();
        for (Data::Index vertex_buffer_index = 0U; vertex_buffer_index < vertex_buffer_set.GetCount(); ++vertex_buffer_index)
        {
            Buffer& vertex_buffer = vertex_buffer_set[vertex_buffer_index];
            beginning_resource_barriers_ptr->AddStateTransition(vertex_buffer, vertex_buffer.GetState(), Resource::State::VertexBuffer);
        }

        return beginning_resource_barriers_ptr;
    }

    void Draw(RenderCommandList& cmd_list, ProgramBindings& program_bindings,
              uint32_t mesh_subset_index = 0, uint32_t instance_count = 1, uint32_t start_instance = 0)
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_LESS_DESCR(mesh_subset_index, m_mesh_subsets.size(), "can not draw mesh subset because its index is out of bounds");

        const Mesh::Subset& mesh_subset = m_mesh_subsets[mesh_subset_index];
        cmd_list.SetProgramBindings(program_bindings);
        cmd_list.SetVertexBuffers(GetVertexBuffers());
        cmd_list.SetIndexBuffer(GetIndexBuffer());
        cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle,
                             mesh_subset.indices.count, mesh_subset.indices.offset,
                             mesh_subset.indices_adjusted ? 0 : mesh_subset.vertices.offset,
                             instance_count, start_instance);
    }

    void Draw(RenderCommandList& cmd_list, const Ptrs<ProgramBindings>& instance_program_bindings,
              ProgramBindings::ApplyBehavior bindings_apply_behavior = ProgramBindings::ApplyBehavior::AllIncremental,
              uint32_t first_instance_index = 0, bool retain_bindings_once = false, bool set_resource_barriers = true)
    {
        Draw(cmd_list, instance_program_bindings.begin(), instance_program_bindings.end(),
             bindings_apply_behavior, first_instance_index, retain_bindings_once, set_resource_barriers);
    }

    void Draw(RenderCommandList& cmd_list,
              const Ptrs<ProgramBindings>::const_iterator& instance_program_bindings_begin,
              const Ptrs<ProgramBindings>::const_iterator& instance_program_bindings_end,
              ProgramBindings::ApplyBehavior bindings_apply_behavior = ProgramBindings::ApplyBehavior::AllIncremental,
              uint32_t first_instance_index = 0, bool retain_bindings_once = false, bool set_resource_barriers = true)
    {
        META_FUNCTION_TASK();
        cmd_list.SetVertexBuffers(GetVertexBuffers(), set_resource_barriers);
        cmd_list.SetIndexBuffer(GetIndexBuffer(), set_resource_barriers);

        for (Ptrs<ProgramBindings>::const_iterator instance_program_bindings_it = instance_program_bindings_begin;
             instance_program_bindings_it != instance_program_bindings_end;
             ++instance_program_bindings_it)
        {
            const Ptr<ProgramBindings>& program_bindings_ptr = *instance_program_bindings_it;
            META_CHECK_ARG_NOT_NULL(program_bindings_ptr);

            const uint32_t instance_index = first_instance_index + static_cast<uint32_t>(std::distance(instance_program_bindings_begin, instance_program_bindings_it));
            const uint32_t subset_index = GetSubsetByInstanceIndex(instance_index);

            META_CHECK_ARG_LESS(subset_index, m_mesh_subsets.size());
            const Mesh::Subset& mesh_subset = m_mesh_subsets[subset_index];

            using namespace magic_enum::bitwise_operators;
            ProgramBindings::ApplyBehavior apply_behavior = bindings_apply_behavior;
            if (!retain_bindings_once || instance_program_bindings_it == instance_program_bindings_begin)
                apply_behavior |= ProgramBindings::ApplyBehavior::RetainResources;
            else
                apply_behavior &= ~ProgramBindings::ApplyBehavior::RetainResources;

            cmd_list.SetProgramBindings(*program_bindings_ptr, apply_behavior);
            cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle,
                                 mesh_subset.indices.count, mesh_subset.indices.offset,
                                 mesh_subset.indices_adjusted ? 0 : mesh_subset.vertices.offset,
                                 1, 0);
        }
    }

    void DrawParallel(const ParallelRenderCommandList& parallel_cmd_list, const Ptrs<ProgramBindings>& instance_program_bindings,
                      ProgramBindings::ApplyBehavior bindings_apply_behavior = ProgramBindings::ApplyBehavior::AllIncremental,
                      bool retain_bindings_once = false, bool set_resource_barriers = true)
    {
        META_FUNCTION_TASK();
        const Ptrs<RenderCommandList>& render_cmd_lists = parallel_cmd_list.GetParallelCommandLists();
        const auto instances_count_per_command_list = static_cast<uint32_t>(Data::DivCeil(instance_program_bindings.size(), render_cmd_lists.size()));

        tf::Taskflow render_task_flow;
        render_task_flow.for_each_index(0U, static_cast<uint32_t>(render_cmd_lists.size()), 1U,
            [this, &render_cmd_lists, instances_count_per_command_list, &instance_program_bindings,
             bindings_apply_behavior, retain_bindings_once, set_resource_barriers](const uint32_t cmd_list_index)
            {
                const Ptr<RenderCommandList>& render_command_list_ptr = render_cmd_lists[cmd_list_index];
                const uint32_t begin_instance_index = cmd_list_index * instances_count_per_command_list;
                const uint32_t end_instance_index   = std::min(begin_instance_index + instances_count_per_command_list,
                                                               static_cast<uint32_t>(instance_program_bindings.size()));

                META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
                Draw(*render_command_list_ptr,
                     instance_program_bindings.begin() + begin_instance_index,
                     instance_program_bindings.begin() + end_instance_index,
                     bindings_apply_behavior, begin_instance_index,
                     retain_bindings_once, set_resource_barriers);
            }
        );
        m_context.GetParallelExecutor().run(render_task_flow).get();
    }

    [[nodiscard]] const std::string&  GetMeshName() const      { return m_mesh_name; }
    [[nodiscard]] Data::Size          GetSubsetsCount() const  { return static_cast<Data::Size>(m_mesh_subsets.size()); }
    [[nodiscard]] Data::Size          GetInstanceCount() const { return static_cast<Data::Size>(m_final_pass_instance_uniforms.size()); }

    [[nodiscard]]
    const UniformsType& GetFinalPassUniforms(Data::Index instance_index = 0U) const
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_LESS(instance_index, m_final_pass_instance_uniforms.size());
        return m_final_pass_instance_uniforms[instance_index];
    }

    void SetFinalPassUniforms(UniformsType&& uniforms, Data::Index instance_index = 0U)
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_LESS(instance_index, m_final_pass_instance_uniforms.size());
        m_final_pass_instance_uniforms[instance_index] = std::move(uniforms);
    }

    [[nodiscard]]
    Data::Size GetUniformsBufferSize() const
    {
        META_FUNCTION_TASK();
        if (m_final_pass_instance_uniforms.empty())
            return 0;
        
        return Buffer::GetAlignedBufferSize(static_cast<Data::Size>(m_final_pass_instance_uniforms.size() * sizeof(m_final_pass_instance_uniforms[0])));
    }

    [[nodiscard]]
    Data::Size GetUniformsBufferOffset(uint32_t instance_index) const
    {
        META_FUNCTION_TASK();
        return static_cast<Data::Size>(
            std::distance(reinterpret_cast<const std::byte*>(m_final_pass_instance_uniforms.data()), // NOSONAR
                          reinterpret_cast<const std::byte*>(&m_final_pass_instance_uniforms[instance_index])) // NOSONAR
        );
    }

    [[nodiscard]]
    const Resource::SubResources& GetFinalPassUniformsSubresources() const
    { return m_final_pass_instance_uniforms_subresources; }

    [[nodiscard]]
    const BufferSet& GetVertexBuffers() const
    {
        META_CHECK_ARG_NOT_NULL(m_vertex_ptr);
        return *m_vertex_ptr;
    }

    [[nodiscard]]
    BufferSet& GetVertexBuffers()
    {
        META_CHECK_ARG_NOT_NULL(m_vertex_ptr);
        return *m_vertex_ptr;
    }

    [[nodiscard]]
    const Buffer& GetIndexBuffer() const
    {
        META_CHECK_ARG_NOT_NULL(m_index_ptr);
        return *m_index_ptr;
    }

    [[nodiscard]]
    Buffer& GetIndexBuffer()
    {
        META_CHECK_ARG_NOT_NULL(m_index_ptr);
        return *m_index_ptr;
    }

protected:
    // Allows to override instance to mesh subset mapping, which is 1:1 by default
    void SetInstanceCount(Data::Size instance_count)
    {
        META_FUNCTION_TASK();
        m_final_pass_instance_uniforms.resize(instance_count);
        m_final_pass_instance_uniforms_subresources = Resource::SubResources{
            { reinterpret_cast<Data::ConstRawPtr>(m_final_pass_instance_uniforms.data()), GetUniformsBufferSize() } // NOSONAR
        };
    }

    [[nodiscard]]
    virtual Data::Index GetSubsetByInstanceIndex(Data::Index instance_index) const { return instance_index; }

private:
    using InstanceUniforms = std::vector<UniformsType, Data::AlignedAllocator<UniformsType, g_uniform_alignment>>;

    const Context&          m_context;
    const std::string       m_mesh_name;
    const Mesh::Subsets     m_mesh_subsets;
    Ptr<BufferSet>          m_vertex_ptr;
    Ptr<Buffer>             m_index_ptr;
    InstanceUniforms        m_final_pass_instance_uniforms; // Actual uniforms buffers are created separately in Frame dependent resources
    Resource::SubResources  m_final_pass_instance_uniforms_subresources;
};

template<typename UniformsType>
class TexturedMeshBuffers : public MeshBuffers<UniformsType>
{
public:
    template<typename VType>
    TexturedMeshBuffers(CommandQueue& render_cmd_queue, const BaseMesh<VType>& mesh_data, const std::string& mesh_name)
        : MeshBuffers<UniformsType>(render_cmd_queue, mesh_data, mesh_name)
    {
        META_FUNCTION_TASK();
        m_subset_textures.resize(1);
    }

    template<typename VType>
    TexturedMeshBuffers(CommandQueue& render_cmd_queue, const UberMesh<VType>& uber_mesh_data, const std::string& mesh_name)
        : MeshBuffers<UniformsType>(render_cmd_queue, uber_mesh_data, mesh_name)
    {
        META_FUNCTION_TASK();
        m_subset_textures.resize(MeshBuffers<UniformsType>::GetSubsetsCount());
    }

    Ptr<Resource::Barriers> CreateBeginningResourceBarriers(Buffer* constants_buffer_ptr = nullptr)
    {
        META_FUNCTION_TASK();
        Ptr<Resource::Barriers> beginning_resource_barriers_ptr = MeshBuffers<UniformsType>::CreateBeginningResourceBarriers(constants_buffer_ptr);
        for (const Ptr<Texture>& texture_ptr : m_subset_textures)
        {
            META_CHECK_ARG_NOT_NULL(texture_ptr);
            beginning_resource_barriers_ptr->AddStateTransition(*texture_ptr, texture_ptr->GetState(), Resource::State::ShaderResource);
        }
        return beginning_resource_barriers_ptr;
    }

    [[nodiscard]]
    const Ptr<Texture>& GetTexturePtr() const
    {
        META_FUNCTION_TASK();
        return GetSubsetTexturePtr(0);
    }

    [[nodiscard]]
    const Ptr<Texture>& GetSubsetTexturePtr(uint32_t subset_index) const
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_LESS(subset_index, MeshBuffers<UniformsType>::GetSubsetsCount());
        return m_subset_textures[subset_index];
    }

    [[nodiscard]]
    const Ptr<Texture>& GetInstanceTexturePtr(uint32_t instance_index = 0) const
    {
        META_FUNCTION_TASK();
        const uint32_t subset_index = this->GetSubsetByInstanceIndex(instance_index);
        return GetSubsetTexturePtr(subset_index);
    }

    [[nodiscard]]
    Texture& GetTexture() const
    {
        META_FUNCTION_TASK();
        const Ptr<Texture>& texture_ptr = GetTexturePtr();
        META_CHECK_ARG_NOT_NULL(texture_ptr);
        return *texture_ptr;
    }

    [[nodiscard]]
    Texture& GetSubsetTexture(uint32_t subset_index) const
    {
        META_FUNCTION_TASK();
        const Ptr<Texture>& texture_ptr = GetSubsetTexturePtr(subset_index);
        META_CHECK_ARG_NOT_NULL(texture_ptr);
        return *texture_ptr;
    }

    [[nodiscard]]
    Texture& GetInstanceTexture(uint32_t instance_index = 0) const
    {
        META_FUNCTION_TASK();
        const Ptr<Texture>& texture_ptr = GetInstanceTexturePtr(instance_index);
        META_CHECK_ARG_NOT_NULL(texture_ptr);
        return *texture_ptr;
    }

    void SetTexture(const Ptr<Texture>& texture_ptr)
    {
        META_FUNCTION_TASK();

        SetSubsetTexture(texture_ptr, 0U);

        if (texture_ptr)
        {
            texture_ptr->SetName(fmt::format("{} Texture", MeshBuffers<UniformsType>::GetMeshName()));
        }
    }
    
    void SetSubsetTexture(const Ptr<Texture>& texture_ptr, uint32_t subset_index)
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_LESS(subset_index, MeshBuffers<UniformsType>::GetSubsetsCount());

        m_subset_textures[subset_index] = texture_ptr;
    }

protected:
    using Textures = std::vector<Ptr<Texture>>;
    
private:
    Textures m_subset_textures;
};

} // namespace Methane::Graphics
