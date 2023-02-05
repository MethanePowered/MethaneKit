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

FILE: Methane/Graphics/RHI/IResource.cpp
Methane resource interface: base class of all GPU resources.

******************************************************************************/

#include <Methane/Graphics/RHI/IResource.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

ResourceAllocationError::ResourceAllocationError(const Rhi::IResource& resource, std::string_view error_message)
    : std::runtime_error(fmt::format("Failed to allocate memory for GPU resource '{}': {}", resource.GetName(), error_message))
    , m_resource(resource)
{ }

} // namespace Methane::Graphics::Rhi
