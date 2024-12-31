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

FILE: Methane/Graphics/SkyBox.h
SkyBox rendering primitive

******************************************************************************/

#pragma once

#include <Methane/Data/Types.h>
#include <Methane/Data/EnumMask.hpp>
#include <Methane/Pimpl.h>

namespace Methane::Graphics::Rhi
{

class Texture;
class Buffer;
class ViewState;
class CommandQueue;
class RenderPattern;
class RenderCommandList;
class ProgramBindings;
struct IProgramArgumentBinding;

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics
{

class Camera;

class SkyBox // NOSONAR - manual copy, move constructors and assignment operators
{
public:
    enum class Option : uint32_t
    {
        None            = 0U,
        DepthEnabled    = 1U << 0U,
        DepthReversed   = 1U << 1U,
        All             = ~0U,
    };

    using OptionMask = Data::EnumMask<Option>;

    struct Settings
    {
        const Camera& view_camera;
        float         scale = 1.F;
        OptionMask    render_options;
        float         lod_bias = 0.F;
    };

    using ProgramBindingsAndUniformArgumentBinding = std::pair<Rhi::ProgramBindings, Rhi::IProgramArgumentBinding*>;

    static Data::Size GetUniformsSize();

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE_NO_INLINE(SkyBox);

    SkyBox(const Rhi::CommandQueue& render_cmd_queue,
           const Rhi::RenderPattern& render_pattern,
           const Rhi::Texture& cube_map_texture,
           const Settings& settings);

    ProgramBindingsAndUniformArgumentBinding CreateProgramBindings(Data::Index frame_index) const;
    void Update(Rhi::IProgramArgumentBinding& uniforms_argument_binding) const;
    void Draw(const Rhi::RenderCommandList& render_cmd_list,
              const Rhi::ProgramBindings& program_bindings,
              const Rhi::ViewState& view_state) const;

    bool IsInitialized() const noexcept { return static_cast<bool>(m_impl_ptr); }

private:
    class Impl;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics
