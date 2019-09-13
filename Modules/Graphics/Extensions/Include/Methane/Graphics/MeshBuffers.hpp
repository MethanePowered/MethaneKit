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
    MeshBuffers(Context& context, const BaseMesh<VType>& mesh_data, const std::string& mesh_name)
        : m_sp_vertex(Buffer::CreateVertexBuffer(context, static_cast<Data::Size>(mesh_data.GetVertexDataSize()),
                                                          static_cast<Data::Size>(mesh_data.GetVertexSize())))
        , m_sp_index( Buffer::CreateIndexBuffer( context, static_cast<Data::Size>(mesh_data.GetIndexDataSize()), 
                                                          PixelFormat::R32Uint))
    {
        m_sp_vertex->SetName(mesh_name + " Vertex Buffer");
        m_sp_vertex->SetData(reinterpret_cast<Data::ConstRawPtr>(mesh_data.GetVertices().data()),
                           static_cast<Data::Size>(mesh_data.GetVertexDataSize()));

        m_sp_index->SetName(mesh_name + " Index Buffer");
        m_sp_index->SetData(reinterpret_cast<Data::ConstRawPtr>(mesh_data.GetIndices().data()),
                          static_cast<Data::Size>(mesh_data.GetIndexDataSize()));
    }

    void Draw(RenderCommandList& cmd_list, Program::ResourceBindings& resource_bindings, uint32_t instance_count)
    {
        cmd_list.SetResourceBindings(resource_bindings);
        cmd_list.SetVertexBuffers({ GetVertexBuffer() });
        cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle, GetIndexBuffer(), instance_count);
    }

    const std::string& GetMeshName() const              { return m_mesh_name; }

    const UniformsType& GetFinalPassUniforms() const    { return m_final_pass_uniforms; }
    void SetFinalPassUniforms(UniformsType& uniforms)   { m_final_pass_uniforms = uniforms; }

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
    const std::string m_mesh_name;
    Buffer::Ptr       m_sp_vertex;
    Buffer::Ptr       m_sp_index;
    UniformsType      m_final_pass_uniforms = { }; // Actual uniforms buffer is created separately in Frame dependent resources
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

    const Texture::Ptr& GetTexturePtr() const { return m_sp_texture; }
    void SetTexture(const Texture::Ptr& sp_texture)
    {
        m_sp_texture = sp_texture;
        m_sp_texture->SetName(GetMeshName() + " Texture");
    }

private:
    Texture::Ptr m_sp_texture;
};

} // namespace Methane::Graphics
