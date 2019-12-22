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

FILE: SkyBox.h
SkyBox rendering primitive

******************************************************************************/

#pragma once

#include "ImageLoader.h"
#include "MeshBuffers.hpp"

#include <Methane/Graphics/Context.h>
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
    using Ptr = std::shared_ptr<SkyBox>;

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

    SkyBox(Context& context, ImageLoader& image_loader, const Settings& settings);

    Program::ResourceBindings::Ptr CreateResourceBindings(const Buffer::Ptr& sp_uniforms_buffer);
    void Resize(const FrameSize& frame_size);
    void Update();
    void Draw(RenderCommandList& cmd_list, MeshBufferBindings& buffer_bindings);

private:
    using TheTexturedMeshBuffers = TexturedMeshBuffers<Uniforms>;

    Settings               m_settings;
    Context&               m_context;
    TheTexturedMeshBuffers m_mesh_buffers;
    Sampler::Ptr           m_sp_texture_sampler;
    RenderState::Ptr       m_sp_state;
    RenderCommandList::Ptr m_sp_command_list;
};

} // namespace Methane::Graphics
