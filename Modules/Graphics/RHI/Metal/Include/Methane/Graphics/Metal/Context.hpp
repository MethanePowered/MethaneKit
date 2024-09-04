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

FILE: Methane/Graphics/Metal/Context.hpp
Metal template implementation of the base context interface.

******************************************************************************/

#pragma once

#include "IContext.h"
#include "Device.hh"
#include "CommandQueue.hh"
#include "Shader.hh"
#include "Program.hh"
#include "ComputeState.hh"
#include "Buffer.hh"
#include "Texture.hh"
#include "Sampler.hh"
#include "ProgramLibrary.hh"
#include "DescriptorManager.hh"

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/RHI/ICommandKit.h>
#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>

#import <Metal/Metal.h>

#include <string>

namespace Methane::Graphics::Metal
{

template<class ContextBaseT, typename = std::enable_if_t<std::is_base_of_v<Base::Context, ContextBaseT>>>
class Context
    : public ContextBaseT
    , public IContext
{
public:
    Context(Base::Device& device, tf::Executor& parallel_executor, const typename ContextBaseT::Settings& settings)
        : ContextBaseT(device, std::make_unique<DescriptorManager>(*this), parallel_executor, settings)
    {
        // NOTE: set low priority of receiver callback, to be called after all other receivers - program bindings,
        // which is required to guarantee that arguments buffer will be allocated before argument bindings initialization
        Data::Emitter<Rhi::IContextCallback>::Connect(
            dynamic_cast<DescriptorManager&>(ContextBaseT::GetDescriptorManager()),
            static_cast<int32_t>(Data::ConnectionPriority::Low)
        );
    }

    // IContext overrides

    [[nodiscard]] Ptr<Rhi::ICommandQueue> CreateCommandQueue(Rhi::CommandListType type) const final
    {
        META_FUNCTION_TASK();
        return std::make_shared<CommandQueue>(*this, type);
    }

    [[nodiscard]] Ptr<Rhi::IShader> CreateShader(Rhi::ShaderType type, const Rhi::ShaderSettings& settings) const final
    {
        META_FUNCTION_TASK();
        return std::make_shared<Shader>(type, *this, settings);
    }

    [[nodiscard]] Ptr<Rhi::IProgram> CreateProgram(const Rhi::ProgramSettings& settings) const final
    {
        META_FUNCTION_TASK();
        return std::make_shared<Program>(*this, settings);
    }

    [[nodiscard]] Ptr<Rhi::IComputeState> CreateComputeState(const Rhi::ComputeStateSettings& settings) const final
    {
        META_FUNCTION_TASK();
        return std::make_shared<ComputeState>(*this, settings);
    }

    [[nodiscard]] Ptr<Rhi::IBuffer> CreateBuffer(const Rhi::BufferSettings& settings) const final
    {
        META_FUNCTION_TASK();
        return std::make_shared<Buffer>(*this, settings);
    }

    [[nodiscard]] Ptr<Rhi::ITexture> CreateTexture(const Rhi::TextureSettings& settings) const final
    {
        META_FUNCTION_TASK();
        return std::make_shared<Texture>(*this, settings);
    }

    [[nodiscard]] Ptr<Rhi::ISampler> CreateSampler(const Rhi::SamplerSettings& settings) const final
    {
        META_FUNCTION_TASK();
        return std::make_shared<Sampler>(*this, settings);
    }

    const Device& GetMetalDevice() const noexcept final
    {
        META_FUNCTION_TASK();
        return static_cast<const Device&>(Base::Context::GetBaseDevice());
    }

    CommandQueue& GetMetalDefaultCommandQueue(Rhi::CommandListType type) final
    {
        META_FUNCTION_TASK();
        return static_cast<CommandQueue&>(Base::Context::GetDefaultCommandKit(type).GetQueue());
    }

    const Ptr<ProgramLibrary>& GetMetalLibrary(std::string_view library_name) const override
    {
        META_FUNCTION_TASK();
        const auto library_by_name_it = m_library_by_name.find(library_name);
        if (library_by_name_it != m_library_by_name.end())
            return library_by_name_it->second;

        return m_library_by_name.try_emplace(std::string(library_name), std::make_shared<ProgramLibrary>(GetMetalDevice(), library_name)).first->second;
    }

    // IObject overrides

    bool SetName(std::string_view name) override
    {
        META_FUNCTION_TASK();
        if (!Base::Context::SetName(name))
            return false;

        m_ns_name = MacOS::ConvertToNsString(name);
        return true;
    }

protected:
    NSString* GetNsName() noexcept { return m_ns_name; }

private:
    using LibraryByName = std::map<std::string, Ptr<ProgramLibrary>, std::less<>>;

    mutable LibraryByName m_library_by_name;
    NSString* m_ns_name;
};

} // namespace Methane::Graphics::Metal
