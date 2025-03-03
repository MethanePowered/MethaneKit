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

FILE: Methane/Graphics/Base/Texture.h
Base implementation of the texture interface.

******************************************************************************/

#pragma once

#include "Resource.h"

#include <Methane/Graphics/RHI/ITexture.h>

namespace Methane::Graphics::Base
{

class Texture
    : public Rhi::ITexture
    , public Resource
{
public:
    Texture(const Context& context, const Settings& settings,
                State initial_state = State::Undefined, Opt<State> auto_transition_source_state_opt = {});

    // ITexture interface
    [[nodiscard]] const Settings&    GetSettings() const override { return m_settings; }
    [[nodiscard]] Data::Size         GetDataSize(Data::MemoryState size_type = Data::MemoryState::Reserved) const noexcept override;
    [[nodiscard]] SubResource::Count GetSubresourceCount() const noexcept final { return m_sub_resource_count; }
    [[nodiscard]] Data::Size         GetSubResourceDataSize(const SubResource::Index& subresource_index) const final;
    [[nodiscard]] Rhi::TextureView   GetTextureView(const SubResource::Index& subresource_index,
                                                    const SubResource::Count& subresource_count = {},
                                                    Opt<Rhi::TextureDimensionType> texture_dimension_type_opt = std::nullopt) final;
    void SetData(Rhi::ICommandQueue&, const SubResources& sub_resources) override;

    static Data::Size GetRequiredMipLevelsCount(const Dimensions& dimensions);

protected:
    // Resource overrides
    Data::Size CalculateSubResourceDataSize(const SubResource::Index& sub_resource_index) const;

    static void ValidateDimensions(DimensionType dimension_type, const Dimensions& dimensions, bool mipmapped);

    void ValidateSubResource(const Rhi::SubResource& sub_resource) const;
    void ValidateSubResource(const SubResource::Index& sub_resource_index, const std::optional<BytesRange>& sub_resource_data_range) const;

private:
    using SubResourceSizes = std::vector<Data::Size>;

    const Settings     m_settings;
    SubResource::Count m_sub_resource_count;
    SubResourceSizes   m_sub_resource_sizes;
};

} // namespace Methane::Graphics::Base
