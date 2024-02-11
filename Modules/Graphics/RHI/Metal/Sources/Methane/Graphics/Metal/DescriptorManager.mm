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

FILE: Methane/Graphics/Metal/DescriptorManager.mm
Metal descriptor manager of the argument buffer

******************************************************************************/

#include <Methane/Graphics/Metal/DescriptorManager.hh>
#include <Methane/Graphics/Metal/Buffer.hh>

namespace Methane::Graphics::Metal
{

DescriptorManager::DescriptorManager(Base::Context& context)
    : Base::DescriptorManager(context)
{
}

void DescriptorManager::CompleteInitialization()
{
    Base::DescriptorManager::CompleteInitialization();
}

void DescriptorManager::Release()
{
    Base::DescriptorManager::Release();
}

} // namespace Methane::Graphics::Metal
