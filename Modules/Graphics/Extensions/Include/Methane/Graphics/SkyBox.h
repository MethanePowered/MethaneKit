/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/SkyBox.h
SkyBox rendering primitive

******************************************************************************/

#pragma once

#include "ImageLoader.h"
#include "MeshBuffers.hpp"

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/Camera.h>
#include <Methane/Graphics/RenderState.h>
#include <Methane/Graphics/Buffer.h>
#include <Methane/Graphics/Program.h>
#include <Methane/Graphics/Sampler.h>
#include <Methane/Graphics/MathTypes.h>

#include <memory>
#include <array>

namespace Methane::Graphics
{

class SkyBox
{
public:
    struct Settings
    {
        const Camera&                  view_camera;
        ImageLoader::CubeFaceResources face_resources;
        float                          scale;
        bool                           depth_enabled = false;
        bool                           depth_reversed = false;
        bool                           mipmapped = false;
        float                          lod_bias = 0.f;
    };

    struct SHADER_STRUCT_ALIGN Uniforms
    {
        SHADER_FIELD_ALIGN Matrix44f mvp_matrix;
    };

    SkyBox(RenderContext& context, ImageLoader& image_loader, const Settings& settings);

    Ptr<ProgramBindings> CreateProgramBindings(const Ptr<Buffer>& sp_uniforms_buffer);
    void Resize(const FrameSize& frame_size);
    void Update();
    void Draw(RenderCommandList& cmd_list, MeshBufferBindings& buffer_bindings);

private:
    struct Vertex
    {
        Mesh::Position position;

        inline static const Mesh::VertexLayout layout = {
            Mesh::VertexField::Position,
        };
    };

    SkyBox(RenderContext& context, ImageLoader& image_loader, const Settings& settings, BaseMesh<Vertex> mesh);

    Settings                      m_settings;
    RenderContext&                m_context;
    TexturedMeshBuffers<Uniforms> m_mesh_buffers;
    Ptr<Sampler>                  m_sp_texture_sampler;
    Ptr<RenderState>              m_sp_state;
};

} // namespace Methane::Graphics
