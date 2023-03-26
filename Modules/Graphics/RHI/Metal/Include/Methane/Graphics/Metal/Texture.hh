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

FILE: Methane/Graphics/Metal/Texture.hh
Metal implementation of the texture interface.

******************************************************************************/

#pragma once

#include "Resource.hh"

#include <Methane/Graphics/Base/Texture.h>
#include <Methane/Graphics/Types.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

class RenderContext;
class TransferCommandList;

class Texture final
    : public Resource<Base::Texture>
{
public:
    Texture(const Base::Context& context, const Settings& settings);

    // IResource interface
    void SetData(Rhi::ICommandQueue& target_cmd_queue, const SubResources& sub_resources) override;
    SubResource GetData(Rhi::ICommandQueue& target_cmd_queue,
                        const SubResource::Index& sub_resource_index = SubResource::Index(),
                        const BytesRangeOpt& data_range = {}) override;

    // IObject interface
    bool SetName(std::string_view name) override;

    void UpdateFrameBuffer();

    const id<MTLTexture>& GetNativeTexture() const { return m_mtl_texture; }

private:
    void GenerateMipLevels(TransferCommandList& transfer_command_list);
    const RenderContext& GetMetalRenderContext() const;

    MTLTextureUsage       GetNativeTextureUsage();
    MTLTextureDescriptor* GetNativeTextureDescriptor();

    id<MTLTexture> m_mtl_texture;
};

} // namespace Methane::Graphics::Metal
