/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Buffer.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Graphics/Program.h>
#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/Mesh.h>
#include <Methane/Graphics/MathTypes.h>

#include <memory>
#include <string>
#include <cassert>

namespace Methane::Graphics
{

template<typename UniformsType>
class MeshBuffers
{
public:
    using Ptr = std::unique_ptr<MeshBuffers<UniformsType>>;

    static inline Data::Size GetUniformsBufferSize()        { return static_cast<Data::Size>(sizeof(UniformsType)); }
    static inline Data::Size GetUniformsAlignedBufferSize() { return Buffer::GetAlignedBufferSize(GetUniformsBufferSize()); }

    template<typename VType>
    MeshBuffers(Context& context, const BaseMesh<VType>& mesh_data, const std::string& mesh_name, const Mesh::Subsets& mesh_subsets = Mesh::Subsets())
        : m_mesh_name(mesh_name)
        , m_mesh_subsets(!mesh_subsets.empty() ? mesh_subsets
                                          : Mesh::Subsets{ Mesh::Subset(mesh_data.GetType(), { 0, mesh_data.GetVertexCount() },
                                                                                             { 0, mesh_data.GetIndexCount()  } ) })
        , m_sp_vertex(Buffer::CreateVertexBuffer(context, static_cast<Data::Size>(mesh_data.GetVertexDataSize()),
                                                          static_cast<Data::Size>(mesh_data.GetVertexSize())))
        , m_sp_index( Buffer::CreateIndexBuffer( context, static_cast<Data::Size>(mesh_data.GetIndexDataSize()), 
                                                          PixelFormat::R32Uint))
    {
        m_final_pass_subset_uniforms.resize(m_mesh_subsets.size());

        m_sp_vertex->SetName(mesh_name + " Vertex Buffer");
        m_sp_vertex->SetData({
            {
                reinterpret_cast<Data::ConstRawPtr>(mesh_data.GetVertices().data()),
                static_cast<Data::Size>(mesh_data.GetVertexDataSize())
            }
        });

        m_sp_index->SetName(mesh_name + " Index Buffer");
        m_sp_index->SetData({
            {
                reinterpret_cast<Data::ConstRawPtr>(mesh_data.GetIndices().data()),
                static_cast<Data::Size>(mesh_data.GetIndexDataSize())
            }
        });
    }

    template<typename VType>
    MeshBuffers(Context& context, const UberMesh<VType>& uber_mesh_data, const std::string& mesh_name)
        : MeshBuffers(context, uber_mesh_data, mesh_name, uber_mesh_data.GetSubsets())
    {
    }

    void Draw(RenderCommandList& cmd_list, const Program::ResourceBindings& resource_bindings,
              uint32_t mesh_subset_index = 0, uint32_t instance_count = 1, uint32_t start_instance = 0)
    {
        if (mesh_subset_index >= m_mesh_subsets.size())
            throw std::invalid_argument("Can not draw mesh subset because its index is out of bounds.");

        const Mesh::Subset& mesh_subset = m_mesh_subsets[mesh_subset_index];
        cmd_list.SetResourceBindings(resource_bindings);
        cmd_list.SetVertexBuffers({ GetVertexBuffer() });
        cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle, GetIndexBuffer(),
                             mesh_subset.indices.count, mesh_subset.indices.offset, mesh_subset.vertices.offset,
                             instance_count, start_instance);
    }

    void Draw(RenderCommandList& cmd_list, const std::vector<Program::ResourceBindings>& subset_resource_bindings)
    {
        if (subset_resource_bindings.size() <= m_mesh_subsets.size())
            throw std::invalid_argument("Resource bindings count is greater than subsets count.");

        cmd_list.SetVertexBuffers({ GetVertexBuffer() });

        const Buffer& index_buffer = GetIndexBuffer();
        uint32_t mesh_subset_index = 0;
        for (const Program::ResourceBindings& resource_bindings : subset_resource_bindings)
        {
            const Mesh::Subset& mesh_subset = m_mesh_subsets[mesh_subset_index++];

            cmd_list.SetResourceBindings(resource_bindings);
            cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle, index_buffer,
                                 mesh_subset.indices.count, mesh_subset.indices.offset, mesh_subset.vertices.offset, 1, 0);
        }
    }

    const std::string&  GetMeshName() const     { return m_mesh_name; }
    uint32_t            GetSubsetsCount() const { return m_mesh_subsets.size(); }

    const UniformsType& GetFinalPassUniforms(uint32_t subset = 0) const
    {
        if (subset >= m_mesh_subsets.size())
            throw std::invalid_argument("Subset index is out of bounds.");

        return m_final_pass_subset_uniforms[subset];
    }

    void  SetFinalPassUniforms(const UniformsType& uniforms, uint32_t subset = 0)
    {
        if (subset >= m_mesh_subsets.size())
            throw std::invalid_argument("Subset index is out of bounds.");

        m_final_pass_subset_uniforms[subset] = uniforms;
    }

protected:
    Buffer& GetVertexBuffer()
    {
        assert(!!m_sp_vertex);
        return *m_sp_vertex;
    }

    Buffer& GetIndexBuffer()
    {
        assert(!!m_sp_index);
        return *m_sp_index;
    }

private:
    using SubsethUniforms = std::vector<UniformsType>;

    const std::string   m_mesh_name;
    const Mesh::Subsets m_mesh_subsets;
    Buffer::Ptr         m_sp_vertex;
    Buffer::Ptr         m_sp_index;
    SubsethUniforms     m_final_pass_subset_uniforms; // Actual uniforms buffer is created separately in Frame dependent resources
};

template<typename UniformsType>
class TexturedMeshBuffers : public MeshBuffers<UniformsType>
{
public:
    using Ptr = std::unique_ptr<TexturedMeshBuffers<UniformsType>>;

    template<typename VType>
    TexturedMeshBuffers(Context& context, const BaseMesh<VType>& mesh_data,
                        const std::string& mesh_name)
        : MeshBuffers<UniformsType>(context, mesh_data, mesh_name)
    {
    }

    template<typename VType>
    TexturedMeshBuffers(Context& context, const UberMesh<VType>& uber_mesh_data, const std::string& mesh_name)
        : MeshBuffers<UniformsType>(context, uber_mesh_data, mesh_name)
    {
    }

    const Texture::Ptr& GetTexturePtr() const { return m_sp_texture; }
    void SetTexture(const Texture::Ptr& sp_texture)
    {
        m_sp_texture = sp_texture;
        m_sp_texture->SetName(MeshBuffers<UniformsType>::GetMeshName() + " Texture");
    }

private:
    Texture::Ptr m_sp_texture;
};

} // namespace Methane::Graphics
