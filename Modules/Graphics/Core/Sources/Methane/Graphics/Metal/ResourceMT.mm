/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/Metal/ResourceMT.mm
Metal implementation of the resource interface.

******************************************************************************/

#include "ResourceMT.hh"
#include "ContextMT.h"
#include "BufferMT.hh"
#include "TextureMT.hh"

#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Instrumentation.h>

#include <vector>

namespace Methane::Graphics
{

Ptr<ResourceBase::Barriers> ResourceBase::Barriers::Create(const Set& barriers)
{
    META_FUNCTION_TASK();
    return std::make_shared<ResourceMT::BarriersMT>(barriers);
}

ResourceMT::ResourceMT(Type type, Usage::Mask usage_mask, ContextBase& context, const DescriptorByUsage& descriptor_by_usage)
    : ResourceBase(type, usage_mask, context, descriptor_by_usage)
{
    META_FUNCTION_TASK();
}

IContextMT& ResourceMT::GetContextMT() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<IContextMT&>(GetContextBase());
}

} // namespace Methane::Graphics
