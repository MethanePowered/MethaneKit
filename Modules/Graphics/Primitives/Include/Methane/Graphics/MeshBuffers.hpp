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

#include "MeshBuffersBase.h"

#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/UberMesh.hpp>
#include <Methane/Graphics/Types.h>
#include <Methane/Data/AlignedAllocator.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>

namespace Methane::Graphics
{

template<typename UniformsType>
class MeshBuffers
    : public MeshBuffersBase
{
private:
    using InstanceUniforms = std::vector<UniformsType, Data::AlignedAllocator<UniformsType, g_uniform_alignment>>;

    // Uniform buffers are created separately in Frame dependent resources
    InstanceUniforms  m_final_pass_instance_uniforms;
    Rhi::SubResource m_final_pass_instance_uniforms_subresource;

public:
    template<typename VertexType>
    MeshBuffers(const Rhi::CommandQueue& render_cmd_queue, const BaseMesh<VertexType>& mesh_data,
                std::string_view mesh_name, const Mesh::Subsets& mesh_subsets = Mesh::Subsets())
        : MeshBuffersBase(render_cmd_queue, mesh_data, mesh_name, mesh_subsets)
    {
        META_FUNCTION_TASK();
        SetInstanceCount(GetSubsetsCount());
    }

    template<typename VertexType>
    MeshBuffers(const Rhi::CommandQueue& render_cmd_queue, const UberMesh<VertexType>& uber_mesh_data, std::string_view mesh_name)
        : MeshBuffers(render_cmd_queue, uber_mesh_data, mesh_name, uber_mesh_data.GetSubsets())
    { }

    [[nodiscard]] Data::Size GetInstanceCount() const noexcept
    {
        return static_cast<Data::Size>(m_final_pass_instance_uniforms.size());
    }

    [[nodiscard]] const Rhi::SubResource& GetFinalPassUniformsSubresource() const
    {
        return m_final_pass_instance_uniforms_subresource;
    }

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
    static constexpr Data::Size GetUniformSize() noexcept
    {
        return static_cast<Data::Size>(sizeof(UniformsType));
    }

    [[nodiscard]]
    Data::Size GetUniformsBufferSize() const
    {
        META_FUNCTION_TASK();
        if (m_final_pass_instance_uniforms.empty())
            return 0U;
        
        return static_cast<Data::Size>(m_final_pass_instance_uniforms.size() * sizeof(m_final_pass_instance_uniforms[0]));
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

protected:
    // Allows to override instance to mesh subset mapping, which is 1:1 by default
    void SetInstanceCount(Data::Size instance_count)
    {
        META_FUNCTION_TASK();
        m_final_pass_instance_uniforms.resize(instance_count);
        m_final_pass_instance_uniforms_subresource = Rhi::SubResource(
            reinterpret_cast<Data::ConstRawPtr>(m_final_pass_instance_uniforms.data()), // NOSONAR
            GetUniformsBufferSize()
        );
    }
};

template<typename UniformsType>
class TexturedMeshBuffers
    : public MeshBuffers<UniformsType>
{
protected:
    using Textures = std::vector<Rhi::Texture>;

private:
    Textures m_subset_textures;

public:
    template<typename VType>
    TexturedMeshBuffers(const Rhi::CommandQueue& render_cmd_queue, const BaseMesh<VType>& mesh_data, const std::string& mesh_name)
        : MeshBuffers<UniformsType>(render_cmd_queue, mesh_data, mesh_name)
    {
        META_FUNCTION_TASK();
        m_subset_textures.resize(1);
    }

    template<typename VType>
    TexturedMeshBuffers(const Rhi::CommandQueue& render_cmd_queue, const UberMesh<VType>& uber_mesh_data, const std::string& mesh_name)
        : MeshBuffers<UniformsType>(render_cmd_queue, uber_mesh_data, mesh_name)
    {
        META_FUNCTION_TASK();
        m_subset_textures.resize(MeshBuffers<UniformsType>::GetSubsetsCount());
    }

    Rhi::ResourceBarriers CreateBeginningResourceBarriers(const Rhi::Buffer* constants_buffer_ptr = nullptr)
    {
        META_FUNCTION_TASK();
        const Rhi::ResourceBarriers beginning_resource_barriers = MeshBuffers<UniformsType>::CreateBeginningResourceBarriers(constants_buffer_ptr);
        for (const Rhi::Texture& texture : m_subset_textures)
        {
            META_CHECK_ARG_TRUE(texture.IsInitialized());
            beginning_resource_barriers.AddStateTransition(texture.GetInterface(), texture.GetState(), Rhi::ResourceState::ShaderResource);
        }
        return beginning_resource_barriers;
    }

    [[nodiscard]]
    const Rhi::Texture& GetTexture() const
    {
        META_FUNCTION_TASK();
        return GetSubsetTexture(0);
    }

    [[nodiscard]]
    const Rhi::Texture& GetSubsetTexture(uint32_t subset_index) const
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_LESS(subset_index, MeshBuffers<UniformsType>::GetSubsetsCount());
        return m_subset_textures[subset_index];
    }

    [[nodiscard]]
    const Rhi::Texture& GetInstanceTexture(uint32_t instance_index = 0) const
    {
        META_FUNCTION_TASK();
        const uint32_t subset_index = this->GetSubsetByInstanceIndex(instance_index);
        return GetSubsetTexture(subset_index);
    }

    void SetTexture(const Rhi::Texture& texture)
    {
        META_FUNCTION_TASK();
        SetSubsetTexture(texture, 0U);
        if (texture.IsInitialized())
        {
            texture.SetName(fmt::format("{} Texture", MeshBuffers<UniformsType>::GetMeshName()));
        }
    }
    
    void SetSubsetTexture(const Rhi::Texture& texture, uint32_t subset_index)
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_LESS(subset_index, MeshBuffers<UniformsType>::GetSubsetsCount());
        m_subset_textures[subset_index] = texture;
    }
};

} // namespace Methane::Graphics
