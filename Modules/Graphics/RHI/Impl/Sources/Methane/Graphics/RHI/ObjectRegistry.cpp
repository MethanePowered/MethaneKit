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

FILE: Methane/Graphics/RHI/ObjectRegistry.cpp
Methane ObjectRegistry PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/ObjectRegistry.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderPass.h>

#include <Methane/Graphics/Base/Object.h>
using ObjectRegistryImpl = Methane::Graphics::Base::ObjectRegistry;

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class ObjectRegistry::Impl
    : public ImplWrapper<IObjectRegistry, ObjectRegistryImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(ObjectRegistry);

ObjectRegistry::ObjectRegistry(const Ptr<IObjectRegistry>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

ObjectRegistry::ObjectRegistry(IObjectRegistry& interface)
    : ObjectRegistry(interface.GetPtr())
{
}

ObjectRegistry::ObjectRegistry(const Refs<ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt)
    : ObjectRegistry(IObjectRegistry::Create(command_list_refs, frame_index_opt))
{
}

void ObjectRegistry::Init(const Refs<ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt)
{
    m_impl_ptr = std::make_unique<Impl>(IObjectRegistry::Create(command_list_refs, frame_index_opt));
}

void ObjectRegistry::Release()
{
    m_impl_ptr.release();
}

bool ObjectRegistry::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IObjectRegistry& ObjectRegistry::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

} // namespace Methane::Graphics::Rhi
