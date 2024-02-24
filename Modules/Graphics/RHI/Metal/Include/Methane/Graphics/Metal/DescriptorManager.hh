/******************************************************************************

Copyright 2024 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/DescriptorManager.hh
Metal descriptor manager of the argument buffer

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/DescriptorManager.h>
#include <Methane/Graphics/RHI/IBuffer.h>
#include <Methane/Graphics/RHI/IContext.h>
#include <Methane/Data/Receiver.hpp>
#include <Methane/Data/RangeSet.hpp>

namespace Methane::Graphics::Metal
{

class DescriptorManager final
    : public Base::DescriptorManager
    , public Data::Receiver<Rhi::IContextCallback>
{
public:
    explicit DescriptorManager(Base::Context& context);

    const Rhi::IBuffer* GetArgumentBuffer() const noexcept { return m_argument_buffer_ptr.get(); }

    // Rhi::IDescriptorManager overrides
    void AddProgramBindings(Rhi::IProgramBindings& program_bindings) override;
    void CompleteInitialization() override;
    void Release() override;

    // Rhi::IContextCallback overrides
    void OnContextCompletingInitialization(Rhi::IContext&) override;
    void OnContextInitialized(Rhi::IContext&) override {}
    void OnContextReleased(Rhi::IContext&) override {}

private:
    using ArgumentsRange = Data::Range<Data::Index>;

    void UpdateArgumentBufferData(Data::Bytes&& argument_buffer_data);

    Ptr<Rhi::IBuffer> m_argument_buffer_ptr;
};

} // namespace Methane::Graphics::Metal
