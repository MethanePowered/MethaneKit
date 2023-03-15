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

FILE: Methane/Graphics/Base/ComputeContext.h
Base implementation of the compute context interface.

******************************************************************************/

#pragma once

#include "Context.h"

#include <Methane/Graphics/RHI/IComputeContext.h>

namespace Methane::Graphics::Base
{

class ComputeContext
    : public Context
    , public Rhi::IComputeContext
{
public:
    ComputeContext(Device& device, UniquePtr<Rhi::IDescriptorManager>&& descriptor_manager_ptr,
                  tf::Executor& parallel_executor, const Settings& settings);

    // Context interface
    void Initialize(Device& device, bool is_callback_emitted = true) override;
    void WaitForGpu(WaitFor wait_for) override;
    [[nodiscard]] OptionMask GetOptions() const noexcept final { return m_settings.options; }

    // IComputeContext interface
    [[nodiscard]] const Settings& GetSettings() const noexcept override { return m_settings; }

protected:
    void ResetWithSettings(const Settings& settings);
    Rhi::IFence& GetComputeFence() const;

    // Context overrides
    bool UploadResources() const override;

private:
    void WaitForGpuComputeComplete();

    Settings m_settings;
};

} // namespace Methane::Graphics::Base
