/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Null/Context.hpp
Null template implementation of the base context interface.

******************************************************************************/

#pragma once

#include "CommandQueue.h"
#include "Shader.h"
#include "Program.h"
#include "ComputeState.h"
#include "Buffer.h"
#include "Texture.h"
#include "Sampler.h"

#include <Methane/Graphics/Base/Device.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/DescriptorManager.h>

namespace Methane::Graphics::Null
{

template<class ContextBaseT, typename = std::enable_if_t<std::is_base_of_v<Base::Context, ContextBaseT>>>
class Context
    : public ContextBaseT
{
public:
    Context(Base::Device& device, tf::Executor& parallel_executor, const typename ContextBaseT::Settings& settings)
        : ContextBaseT(device, std::make_unique<Base::DescriptorManager>(*this), parallel_executor, settings)
    {
    }

    // IContext overrides

    [[nodiscard]] Ptr<Rhi::ICommandQueue> CreateCommandQueue(Rhi::CommandListType type) const final
    {
        return std::make_shared<CommandQueue>(*this, type);
    }

    [[nodiscard]] Ptr<Rhi::IShader> CreateShader(Rhi::ShaderType type, const Rhi::ShaderSettings& settings) const final
    {
        return std::make_shared<Shader>(type, *this, settings);
    }

    [[nodiscard]] Ptr<Rhi::IProgram> CreateProgram(const Rhi::ProgramSettings& settings) const final
    {
        return std::make_shared<Program>(*this, settings);
    }

    [[nodiscard]] Ptr<Rhi::IComputeState> CreateComputeState(const Rhi::ComputeStateSettings& settings) const final
    {
        return std::make_shared<ComputeState>(*this, settings);
    }

    [[nodiscard]] Ptr<Rhi::IBuffer> CreateBuffer(const Rhi::BufferSettings& settings) const final
    {
        return std::make_shared<Buffer>(*this, settings);
    }

    [[nodiscard]] Ptr<Rhi::ITexture> CreateTexture(const Rhi::TextureSettings& settings) const final
    {
        return std::make_shared<Texture>(*this, settings);
    }

    [[nodiscard]] Ptr<Rhi::ISampler> CreateSampler(const Rhi::SamplerSettings& settings) const final
    {
        return std::make_shared<Sampler>(*this, settings);
    }
};

} // namespace Methane::Graphics::Null
