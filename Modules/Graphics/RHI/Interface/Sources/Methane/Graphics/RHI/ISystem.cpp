/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/ISystem.cpp
Methane system interface to query graphics devices.

******************************************************************************/

#include <Methane/Graphics/RHI/ISystem.h>

namespace Methane::Graphics::Rhi
{

NativeApi ISystem::GetNativeApi() noexcept
{
#if defined METHANE_GFX_METAL
    return NativeApi::Metal;
#elif defined METHANE_GFX_DIRECTX
    return NativeApi::DirectX;
#elif defined METHANE_GFX_VULKAN
    return NativeApi::Vulkan;
#else
    return NativeApi::Undefined;
#endif
}

} // namespace Methane::Graphics::Rhi
