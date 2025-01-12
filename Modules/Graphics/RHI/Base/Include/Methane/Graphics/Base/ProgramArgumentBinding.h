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

FILE: Methane/Graphics/Base/ProgramArgumentBinding.h
Base implementation of the program argument binding interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/IProgramBindings.h>
#include <Methane/Graphics/Base/RootConstantBuffer.h>
#include <Methane/Data/Emitter.hpp>

namespace Methane::Graphics::Base
{

class Context;
class Program;
class ProgramBindings;
class RootConstantAccessor;
class RootConstantBuffer;

class ProgramArgumentBinding // NOSONAR - destructor is defaulted in CPP, to allow deleting of incomplete type
    : public Rhi::IProgramArgumentBinding
    , public Data::Emitter<Rhi::IProgramArgumentBindingCallback>
    , public std::enable_shared_from_this<ProgramArgumentBinding>
    , protected Data::Receiver<IRootConstantBufferCallback>
{
public:
    ProgramArgumentBinding(const Context& context, const Settings& settings);
    ProgramArgumentBinding(const ProgramArgumentBinding& other);
    ~ProgramArgumentBinding() override;

    // Base::ProgramArgumentBinding interface
    [[nodiscard]] virtual Ptr<ProgramArgumentBinding> CreateCopy() const = 0;
    virtual void MergeSettings(const ProgramArgumentBinding& other);

    // IArgumentBinding interface
    const Settings&           GetSettings() const noexcept override     { return m_settings; }
    const Rhi::ResourceViews& GetResourceViews() const noexcept final   { return m_resource_views; }
    bool                      SetResourceViews(Rhi::ResourceViewSpan resource_views) override;
    bool                      SetResourceViews(const Rhi::ResourceViews& resource_views) final;
    bool                      SetResourceView(const Rhi::ResourceView& resource_view) final;
    Rhi::RootConstant         GetRootConstant() const final;
    bool                      SetRootConstant(const Rhi::RootConstant& root_constant) override;
    explicit operator std::string() const final;

    bool GetEmitCallbackEnabled() const noexcept { return m_emit_callback_enabled; }
    void SetEmitCallbackEnabled(bool enabled)    { m_emit_callback_enabled = enabled; }

    Ptr<ProgramArgumentBinding> GetPtr() { return shared_from_this(); }
    RootConstantAccessor*       GetRootConstantAccessorPtr() const { return m_root_constant_accessor_ptr.get(); }

    void Initialize(Program& program, Data::Index frame_index);
    bool IsAlreadyApplied(const Rhi::IProgram& program,
                          const ProgramBindings& applied_program_bindings,
                          bool check_binding_value_changes = true) const;

protected:
    const Context& GetContext() const noexcept { return m_context; }

    // IRootConstantBufferCallback overrides...
    void OnRootConstantBufferChanged(RootConstantBuffer&, const Ptr<Rhi::IBuffer>&) override;

    virtual bool UpdateRootConstantResourceViews();

private:
    const Context&                  m_context;
    Settings                        m_settings;
    Rhi::ResourceViews              m_resource_views;
    UniquePtr<RootConstantAccessor> m_root_constant_accessor_ptr;
    bool                            m_emit_callback_enabled = true;
};

} // namespace Methane::Graphics::Base
