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

FILE: Methane/Graphics/Metal/TextureMT.hh
Metal implementation of the texture interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/TextureBase.h>
#include <Methane/Graphics/Types.h>

#import <Metal/Metal.h>

namespace Methane::Graphics
{

class RenderContextMT;

class TextureMT final : public TextureBase
{
public:
    TextureMT(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());
    ~TextureMT() override;

    // Resource interface
    void SetData(const SubResources& sub_resources) override;
    Data::Size GetDataSize() const override;

    // Object interface
    void SetName(const std::string& name) override;

    void UpdateFrameBuffer();

    const id<MTLTexture>& GetNativeTexture() const { return m_mtl_texture; }

private:
    void GenerateMipLevels();
    RenderContextMT& GetRenderContextMT();

    MTLTextureUsage       GetNativeTextureUsage();
    MTLTextureDescriptor* GetNativeTextureDescriptor();

    id<MTLTexture> m_mtl_texture;
};

} // namespace Methane::Graphics
