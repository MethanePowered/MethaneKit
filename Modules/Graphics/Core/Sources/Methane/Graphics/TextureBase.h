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

FILE: Methane/Graphics/TextureBase.h
Base implementation of the texture interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Texture.h>

#include "Native/ResourceNT.h"

namespace Methane::Graphics
{

class TextureBase
    : public Texture
    , public ResourceNT
{
public:
    TextureBase(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage);

    // Texture interface
    const Settings& GetSettings() const override { return m_settings; }
    uint32_t        GetMipLevelsCount() const override;

protected:
    uint32_t GetRequiredSubresourceCount() const;
    static void ValidateDimensions(DimensionType dimension_type, const Dimensions& dimensions, bool mipmapped);

private:
    const Settings m_settings;
};

} // namespace Methane::Graphics
