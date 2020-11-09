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
#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/ParallelRenderCommandList.h>
#include <Methane/Graphics/Mesh/UberMesh.hpp>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/TypeConverters.hpp>
#include <Methane/Data/AlignedAllocator.hpp>
#include <Methane/Data/Math.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <taskflow/taskflow.hpp>

#include <memory>
#include <string>

namespace Methane::Graphics
{
    
struct MeshBufferBindings
{
    Ptr<Buffer>           uniforms_buffer_ptr;
    Ptrs<ProgramBindings> program_bindings_per_instance;
};

template<typename UniformsType>
class MeshBuffers
{
public:
    template<typename VType>
    MeshBuffers(RenderContext& context, const BaseMesh<VType>& mesh_data, const std::string& mesh_name, const Mesh::Subsets& mesh_subsets = Mesh::Subsets())
        : m_render_context(context)
        , m_mesh_name(mesh_name)
        , m_mesh_subsets(!mesh_subsets.empty() ? mesh_subsets
                                               : Mesh::Subsets{ Mesh::Subset(mesh_data.GetType(), { 0, mesh_data.GetVertexCount() },
                                                                                                  { 0, mesh_data.GetIndexCount()  }, true ) })
    {
        META_FUNCTION_TASK();
        SetInstanceCount(static_cast<Data::Size>(m_mesh_subsets.size()));

        Ptr<Buffer> vertex_buffer_ptr = Buffer::CreateVertexBuffer(context,
                                                                   static_cast<Data::Size>(mesh_data.GetVertexDataSize()),
                                                                   static_cast<Data::Size>(mesh_data.GetVertexSize()));
        vertex_buffer_ptr->SetName(mesh_name + " Vertex Buffer");
        vertex_buffer_ptr->SetData({
            {
                reinterpret_cast<Data::ConstRawPtr>(mesh_data.GetVertices().data()),
                static_cast<Data::Size>(mesh_data.GetVertexDataSize())
            }
        });
        m_vertex_ptr = BufferSet::CreateVertexBuffers({ *vertex_buffer_ptr });

        m_index_ptr = Buffer::CreateIndexBuffer(context, static_cast<Data::Size>(mesh_data.GetIndexDataSize()), GetIndexFormat(mesh_data.GetIndex(0)));
        m_index_ptr->SetName(mesh_name + " Index Buffer");
        m_index_ptr->SetData({
            {
                reinterpret_cast<Data::ConstRawPtr>(mesh_data.GetIndices().data()),
                static_cast<Data::Size>(mesh_data.GetIndexDataSize())
            }
        });
    }

    template<typename VType>
    MeshBuffers(RenderContext& context, const UberMesh<VType>& uber_mesh_data, const std::string& mesh_name)
        : MeshBuffers(context, uber_mesh_data, mesh_name, uber_mesh_data.GetSubsets())
    {
        META_FUNCTION_TASK();
    }
    
    virtual ~MeshBuffers() = default;

    RenderContext& GetRenderContext() const noexcept { return m_render_context; }

    void Draw(RenderCommandList& cmd_list, ProgramBindings& program_bindings,
              uint32_t mesh_subset_index = 0, uint32_t instance_count = 1, uint32_t start_instance = 0)
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_LESS_DESCR(mesh_subset_index, m_mesh_subsets.size(), "can not draw mesh subset because its index is out of bounds");

        const Mesh::Subset& mesh_subset = m_mesh_subsets[mesh_subset_index];
        cmd_list.SetProgramBindings(program_bindings);
        cmd_list.SetVertexBuffers(GetVertexBuffers());
        cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle, GetIndexBuffer(),
                             mesh_subset.indices.count, mesh_subset.indices.offset,
                             mesh_subset.indices_adjusted ? 0 : mesh_subset.vertices.offset,
                             instance_count, start_instance);
    }

    void Draw(RenderCommandList& cmd_list, const Ptrs<ProgramBindings>& instance_program_bindings, uint32_t first_instance_index = 0)
    {
        Draw(cmd_list, instance_program_bindings.begin(), instance_program_bindings.end(), first_instance_index);
    }

    void Draw(RenderCommandList& cmd_list,
              const Ptrs<ProgramBindings>::const_iterator& instance_program_bindings_begin,
              const Ptrs<ProgramBindings>::const_iterator& instance_program_bindings_end,
              ProgramBindings::ApplyBehavior::Mask bindings_apply_behavior = ProgramBindings::ApplyBehavior::AllIncremental,
              uint32_t first_instance_index = 0)
    {
        META_FUNCTION_TASK();

        cmd_list.SetVertexBuffers(GetVertexBuffers());

        Buffer& index_buffer = GetIndexBuffer();
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

            cmd_list.SetProgramBindings(*program_bindings_ptr, bindings_apply_behavior);
            cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle, index_buffer,
                                 mesh_subset.indices.count, mesh_subset.indices.offset,
                                 mesh_subset.indices_adjusted ? 0 : mesh_subset.vertices.offset,
                                 1, 0);
        }
    }

    void DrawParallel(ParallelRenderCommandList& parallel_cmd_list, const Ptrs<ProgramBindings>& instance_program_bindings,
                      ProgramBindings::ApplyBehavior::Mask bindings_apply_behavior = ProgramBindings::ApplyBehavior::AllIncremental)
    {
        META_FUNCTION_TASK();

        const Ptrs<RenderCommandList>& render_cmd_lists = parallel_cmd_list.GetParallelCommandLists();
        const uint32_t instances_count_per_command_list = static_cast<uint32_t>(Data::DivCeil(instance_program_bindings.size(), render_cmd_lists.size()));

        tf::Taskflow render_task_flow;
        render_task_flow.for_each_index_guided(0, static_cast<int>(render_cmd_lists.size()), 1,
            [&](const int cmd_list_index)
            {
                META_FUNCTION_TASK();
                const Ptr<RenderCommandList>& render_command_list_ptr = render_cmd_lists[cmd_list_index];
                const uint32_t begin_instance_index = static_cast<uint32_t>(cmd_list_index * instances_count_per_command_list);
                const uint32_t end_instance_index   = std::min(begin_instance_index + instances_count_per_command_list,
                                                               static_cast<uint32_t>(instance_program_bindings.size()));

                META_CHECK_ARG_NOT_NULL(render_command_list_ptr);
                Draw(*render_command_list_ptr,
                     instance_program_bindings.begin() + begin_instance_index,
                     instance_program_bindings.begin() + end_instance_index,
                     bindings_apply_behavior, begin_instance_index);
            },
            Data::GetParallelChunkSizeAsInt(render_cmd_lists.size(), 5)
        );
        m_render_context.GetParallelExecutor().run(render_task_flow).get();
    }

    const std::string&  GetMeshName() const      { return m_mesh_name; }
    Data::Size          GetSubsetsCount() const  { return static_cast<Data::Size>(m_mesh_subsets.size()); }
    Data::Size          GetInstanceCount() const { return static_cast<Data::Size>(m_final_pass_instance_uniforms.size()); }

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

    Data::Size GetUniformsBufferSize() const
    {
        META_FUNCTION_TASK();
        if (m_final_pass_instance_uniforms.empty())
            return 0;
        
        return Buffer::GetAlignedBufferSize(static_cast<Data::Size>(m_final_pass_instance_uniforms.size() * sizeof(m_final_pass_instance_uniforms[0])));
    }

    const Resource::SubResources& GetFinalPassUniformsSubresources() const { return m_final_pass_instance_uniforms_subresources; }

protected:
    // Allows to override instance to mesh subset mapping, which is 1:1 by default
    void SetInstanceCount(Data::Size instance_count)
    {
        META_FUNCTION_TASK();
        m_final_pass_instance_uniforms.resize(instance_count);
        m_final_pass_instance_uniforms_subresources = Resource::SubResources{
            { reinterpret_cast<Data::ConstRawPtr>(m_final_pass_instance_uniforms.data()), GetUniformsBufferSize() }
        };
    }

    virtual Data::Index GetSubsetByInstanceIndex(Data::Index instance_index) const { return instance_index; }

    const BufferSet& GetVertexBuffers() const
    {
        META_CHECK_ARG_NOT_NULL(m_vertex_ptr);
        return *m_vertex_ptr;
    }

    BufferSet& GetVertexBuffers()
    {
        META_CHECK_ARG_NOT_NULL(m_vertex_ptr);
        return *m_vertex_ptr;
    }

    Buffer& GetIndexBuffer()
    {
        META_CHECK_ARG_NOT_NULL(m_index_ptr);
        return *m_index_ptr;
    }
    
    Data::Size GetUniformsBufferOffset(uint32_t instance_index) const
    {
        META_FUNCTION_TASK();
        return static_cast<Data::Size>(
            reinterpret_cast<const char*>(&m_final_pass_instance_uniforms[instance_index]) -
            reinterpret_cast<const char*>(m_final_pass_instance_uniforms.data())
        );
    }

private:
    using InstanceUniforms = std::vector<UniformsType, Data::AlignedAllocator<UniformsType, SHADER_STRUCT_ALIGNMENT>>;

    RenderContext&         m_render_context;
    const std::string      m_mesh_name;
    const Mesh::Subsets    m_mesh_subsets;
    Ptr<BufferSet>         m_vertex_ptr;
    Ptr<Buffer>            m_index_ptr;
    InstanceUniforms       m_final_pass_instance_uniforms; // Actual uniforms buffers are created separately in Frame dependent resources
    Resource::SubResources m_final_pass_instance_uniforms_subresources;
};

template<typename UniformsType>
class TexturedMeshBuffers : public MeshBuffers<UniformsType>
{
public:
    template<typename VType>
    TexturedMeshBuffers(RenderContext& context, const BaseMesh<VType>& mesh_data, const std::string& mesh_name)
        : MeshBuffers<UniformsType>(context, mesh_data, mesh_name)
    {
        META_FUNCTION_TASK();
        m_subset_textures.resize(1);
    }

    template<typename VType>
    TexturedMeshBuffers(RenderContext& context, const UberMesh<VType>& uber_mesh_data, const std::string& mesh_name)
        : MeshBuffers<UniformsType>(context, uber_mesh_data, mesh_name)
    {
        META_FUNCTION_TASK();
        m_subset_textures.resize(MeshBuffers<UniformsType>::GetSubsetsCount());
    }

    const Ptr<Texture>& GetTexturePtr() const
    {
        META_FUNCTION_TASK();
        return GetSubsetTexturePtr(0);
    }

    const Ptr<Texture>& GetSubsetTexturePtr(uint32_t subset_index) const
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_LESS(subset_index, MeshBuffers<UniformsType>::GetSubsetsCount());

        return m_subset_textures[subset_index];
    }

    const Ptr<Texture>& GetInstanceTexturePtr(uint32_t instance_index = 0) const
    {
        META_FUNCTION_TASK();

        const uint32_t subset_index = this->GetSubsetByInstanceIndex(instance_index);
        return GetSubsetTexturePtr(subset_index);
    }

    void SetTexture(const Ptr<Texture>& texture_ptr)
    {
        META_FUNCTION_TASK();

        SetSubsetTexture(texture_ptr, 0U);

        if (texture_ptr)
        {
            texture_ptr->SetName(MeshBuffers<UniformsType>::GetMeshName() + " Texture");
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
