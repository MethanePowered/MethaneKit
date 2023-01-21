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

FILE: Methane/Graphics/RHI/Device.cpp
Methane Device PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/Device.h>
#include <Methane/Graphics/RHI/RenderContext.h>

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <Device.hh>
#else
#include <Device.h>
#endif

#include <algorithm>
#include <iterator>

namespace Methane::Graphics::Rhi
{

META_PIMPL_METHODS_IMPLEMENT(Device);

Device::Device(const Ptr<IDevice>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

Device::Device(IDevice& interface_ref)
    : Device(interface_ref.GetDerivedPtr<IDevice>())
{
}

IDevice& Device::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IDevice> Device::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool Device::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view Device::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

RenderContext Device::CreateRenderContext(const Platform::AppEnvironment& env, tf::Executor& parallel_executor, const RenderContextSettings& settings) const
{
    return RenderContext(GetImpl(m_impl_ptr).CreateRenderContext(env, parallel_executor, settings));
}

const std::string& Device::GetAdapterName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetAdapterName();
}

bool Device::IsSoftwareAdapter() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).IsSoftwareAdapter();
}

const DeviceCaps& Device::GetCapabilities() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetCapabilities();
}

std::string Device::ToString() const
{
    return GetImpl(m_impl_ptr).ToString();
}

void Device::Connect(Data::Receiver<IDeviceCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IDeviceCallback>::Connect(receiver);
}

void Device::Disconnect(Data::Receiver<IDeviceCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IDeviceCallback>::Disconnect(receiver);
}

} // namespace Methane::Graphics::Rhi
