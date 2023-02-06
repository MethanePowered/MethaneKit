/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/Buffer.hh
Metal implementation of the buffer interface.

******************************************************************************/

#pragma once

#include "Resource.hh"

#include <Methane/Graphics/Base/Buffer.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

class Buffer final
    : public Resource<Base::Buffer>
{
public:
    Buffer(const Base::Context& context, const Settings& settings);

    // IResource interface
    void SetData(const SubResources& sub_resources, Rhi::ICommandQueue& target_cmd_queue) override;

    // IObject interface
    bool SetName(std::string_view name) override;
    
    const id<MTLBuffer>& GetNativeBuffer() const noexcept { return m_mtl_buffer; }
    MTLIndexType         GetNativeIndexType() const noexcept;

private:
    void SetDataToManagedBuffer(const SubResources& sub_resources);
    void SetDataToPrivateBuffer(const SubResources& sub_resources);

    id<MTLBuffer> m_mtl_buffer;
};

} // namespace Methane::Graphics::Metal
