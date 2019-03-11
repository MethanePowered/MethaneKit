/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/TextureMT.h
Metal implementation of the texture interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/TextureBase.h>

#import <Metal/Metal.h>

namespace Methane
{
namespace Graphics
{

class TextureMT : public TextureBase
{
public:
    using Ptr = std::shared_ptr<TextureMT>;

    TextureMT(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());
    virtual ~TextureMT() override;

    // Resource interface
    virtual void SetData(Data::ConstRawPtr p_data, Data::Size data_size) override;
    virtual Data::Size GetDataSize() const override;

    // Object interface
    virtual void SetName(const std::string& name) override;

    void UpdateFrameBuffer();

    const id<MTLTexture>& GetNativeTexture() const { return m_mtl_texture; }

protected:
    MTLTextureUsage       GetNativeTextureUsage();
    MTLTextureDescriptor* GetNativeTextureDescriptor();

    id<MTLTexture> m_mtl_texture;
};

} // namespace Graphics
} // namespace Methane
