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

FILE: Methane/Graphics/Base/ComputeState.h
Base implementation of the compute state interface.

******************************************************************************/

#pragma once

#include "Object.h"

#include <Methane/Graphics/RHI/IComputeState.h>

namespace Methane::Graphics::Base
{

class ComputeCommandList;

class ComputeState
    : public Object
    , public Rhi::IComputeState
{
public:
    ComputeState(const Rhi::IContext& context, const Settings& settings);

    // IComputeState overrides
    const Settings& GetSettings() const noexcept override { return m_settings; }
    void Reset(const Settings& settings) override;

    // ComputeState interface
    virtual void Apply(ComputeCommandList& command_list) = 0;

    const Rhi::IContext& GetContext() const noexcept { return m_context; }

protected:
    Rhi::IProgram& GetProgram();

private:
    const Rhi::IContext& m_context;
    Settings             m_settings;
};

} // namespace Methane::Graphics::Base
