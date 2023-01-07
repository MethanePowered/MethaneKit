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

FILE: Methane/Graphics/Null/Buffer.cpp
Null implementation of the buffer interface.

******************************************************************************/

#include <Methane/Graphics/Null/Buffer.h>

#include <Methane/Graphics/Base/Context.h>

#include <iterator>

namespace Methane::Graphics::Rhi
{

Ptr<IBuffer> IBuffer::Create(const IContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<Null::Buffer>(dynamic_cast<const Base::Context&>(context), settings);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Null
{

Buffer::Buffer(const Base::Context& context, const Settings& settings)
    : Resource(context, settings)
{
}

} // namespace Methane::Graphics::Null
