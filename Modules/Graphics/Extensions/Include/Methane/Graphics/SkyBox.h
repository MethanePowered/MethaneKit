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
#include <Methane/Graphics/RenderState.h>
#include <Methane/Graphics/MathTypes.h>

#include <memory>
#include <array>
#include <string>

namespace Methane::Graphics
{

class SkyBox
{
public:
    using Ptr = std::shared_ptr<SkyBox>;

    struct Settings
    {
        ImageLoader::CubeFaceResources face_resources;
    };

    struct SHADER_STRUCT_ALIGN MeshUniforms
    {
        SHADER_FIELD_ALIGN Matrix44f mvp_matrix;
    };

    SkyBox(Context& context, ImageLoader& image_loader, const Settings& settings);

    const Program::Ptr& GetProgramPtr() const               { return m_sp_state->GetSettings().sp_program; }
    const Texture::Ptr& GetTexturePtr() const               { return m_mesh_buffers.GetTexturePtr(); }
    const MeshUniforms& GetFinalPassUniforms() const        { return m_mesh_buffers.GetFinalPassUniforms(); }
    void SetFinalPassUniforms(const MeshUniforms& uniforms) { m_mesh_buffers.SetFinalPassUniforms(uniforms); }

    void Resize(const FrameSize& frame_size);
    void Draw(RenderCommandList& cmd_list, Program::ResourceBindings& resource_bindings);

private:
    using TexturedMeshBuffers = TexturedMeshBuffers<MeshUniforms>;

    Settings               m_settings;
    Context&               m_context;
    TexturedMeshBuffers    m_mesh_buffers;
    RenderState::Ptr       m_sp_state;
    RenderCommandList::Ptr m_sp_command_list;
};

} // namespace Methane::Graphics
