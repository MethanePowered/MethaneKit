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
#include <Methane/Graphics/RHI/IProgram.h>
#include <Methane/Data/Receiver.hpp>
#include <Methane/Data/RangeSet.hpp>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>

#include <mutex>
#include <array>

namespace Methane::Graphics::Metal
{

class ProgramBindings;

class DescriptorManager final
    : public Base::DescriptorManager
    , public Data::Receiver<Rhi::IContextCallback>
{
public:
    using ArgumentsRange    = Data::Range<Data::Index>;
    using ArgumentsRangeSet = Data::RangeSet<Data::Index>;

    class ArgumentsBuffer
    {
        friend class DescriptorManager;

    public:
        ArgumentsBuffer(const Base::Context& context, Rhi::ProgramArgumentAccessType access_type);

        Rhi::ProgramArgumentAccessType GetAccessType() const noexcept { return m_access_type; }

        Data::Index         GetIndex() const noexcept      { return static_cast<Data::Index>(m_access_type); }
        Data::Size          GetDataSize() const noexcept   { return static_cast<Data::Size>(m_data.size()); }
        const Data::Bytes&  GetData() const noexcept       { return m_data; }
        Data::Byte*         GetDataPtr() noexcept          { return m_data.data(); }
        const Rhi::IBuffer* GetBuffer() const;
        Rhi::IBuffer*       GetBuffer();

        ArgumentsRange ReserveRange(Data::Size range_size);
        void ReleaseRange(const ArgumentsRange& range);
        void Update();

    private:
        void CreateBuffer() const;
        void Release();

        const Base::Context&                 m_context;
        const Rhi::ProgramArgumentAccessType m_access_type;
        Data::Bytes                          m_data;
        ArgumentsRangeSet                    m_free_ranges;
        mutable Ptr<Rhi::IBuffer>            m_buffer_ptr;
        TracyLockable(std::mutex,            m_mutex);
    };

    explicit DescriptorManager(Base::Context& context);

    ArgumentsBuffer& GetArgumentsBuffer(Rhi::ProgramArgumentAccessType access_type) noexcept;
    const ArgumentsBuffer& GetArgumentsBuffer(Rhi::ProgramArgumentAccessType access_type) const noexcept;

    // Rhi::IDescriptorManager overrides
    void CompleteInitialization() override { /* Replaced with initialization in OnContextUploadingResources() */ }
    void AddProgramBindings(Rhi::IProgramBindings& program_bindings) override;
    void RemoveProgramBindings(Rhi::IProgramBindings& program_bindings) override;
    void Release() override;

    // Rhi::IContextCallback overrides
    void OnContextUploadingResources(Rhi::IContext&) override;
    void OnContextInitialized(Rhi::IContext&) override {}
    void OnContextReleased(Rhi::IContext&) override {}

private:
    using ArgumentsBufferByAccessType = std::array<ArgumentsBuffer, magic_enum::enum_count<Rhi::ProgramArgumentAccessType>()>;

    ArgumentsBufferByAccessType m_arguments_buffer_by_access_type;
};

} // namespace Methane::Graphics::Metal
