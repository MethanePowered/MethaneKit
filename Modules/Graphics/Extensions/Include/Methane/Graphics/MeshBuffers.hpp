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
#include <Methane/Graphics/Mesh.h>
#include <Methane/Graphics/MathTypes.h>

#include <memory>

namespace Methane
{
namespace Graphics
{

template<typename UniformsType>
struct MeshBuffers
{
    using Ptr = std::unique_ptr<MeshBuffers<UniformsType>>;

    UniformsType uniforms = { };

    Buffer::Ptr sp_vertex;
    Buffer::Ptr sp_index;

    template<typename VType>
    MeshBuffers(Context& context, const BaseMesh<VType>& mesh_data, const std::string& mesh_name)
        : sp_vertex(Buffer::CreateVertexBuffer(context, static_cast<Data::Size>(mesh_data.GetVertexDataSize()),
                                                        static_cast<Data::Size>(mesh_data.GetVertexSize())))
        , sp_index( Buffer::CreateIndexBuffer( context, static_cast<Data::Size>(mesh_data.GetIndexDataSize()), 
                                                        PixelFormat::R32Uint))
    {
        sp_vertex->SetName(mesh_name + " Vertex Buffer");
        sp_vertex->SetData(reinterpret_cast<Data::ConstRawPtr>(mesh_data.GetVertices().data()),
                           static_cast<Data::Size>(mesh_data.GetVertexDataSize()));

        sp_index->SetName(mesh_name + " Index Buffer");
        sp_index->SetData(reinterpret_cast<Data::ConstRawPtr>(mesh_data.GetIndices().data()),
                          static_cast<Data::Size>(mesh_data.GetIndexDataSize()));
    }
};

template<typename UniformsType>
struct TexturedMeshBuffers : MeshBuffers<UniformsType>
{
    using Ptr = std::unique_ptr<TexturedMeshBuffers<UniformsType>>;
    
    Texture::Ptr sp_texture;

    template<typename VType>
    TexturedMeshBuffers(Context& context, const BaseMesh<VType>& mesh_data,
                        ImageLoader& image_loader, const std::string& texture_path,
                        const std::string& mesh_name)
        : MeshBuffers<UniformsType>(context, mesh_data, mesh_name)
        , sp_texture(image_loader.CreateImageTexture(context, texture_path))
    {
        sp_texture->SetName(mesh_name + " Texture");
    }
};


} // namespace Samples
} // namespace Methane
