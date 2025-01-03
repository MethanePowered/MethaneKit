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

FILE: Methane/Graphics/RHI/IComputeContext.cpp
Methane render context interface: represents graphics device and swap chain,
provides basic multi-frame rendering synchronization and frame presenting APIs.

******************************************************************************/

#include <Methane/Graphics/RHI/IComputeContext.h>
#include <Methane/Graphics/RHI/IDevice.h>
#include <Methane/Graphics/RHI/ICommandList.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

Ptr<IComputeContext> IComputeContext::Create(IDevice& device, tf::Executor& parallel_executor, const Settings& settings)
{
    META_FUNCTION_TASK();
    return device.CreateComputeContext(parallel_executor, settings);
}

ICommandKit& IComputeContext::GetComputeCommandKit() const
{
    return GetDefaultCommandKit(CommandListType::Compute);
}

} // namespace Methane::Graphics::Rhi
