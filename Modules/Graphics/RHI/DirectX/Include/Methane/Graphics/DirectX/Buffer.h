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

FILE: Methane/Graphics/DirectX/Buffer.h
DirectX 12 implementation of the buffer interface.

******************************************************************************/

#pragma once

#include "Resource.hpp"

#include <Methane/Graphics/Base/Buffer.h>

#include <directx/d3d12.h>

namespace Methane::Graphics::DirectX
{

class Buffer final // NOSONAR - inheritance hierarchy is greater than 5
    : public Resource<Base::Buffer>
{
public:
    Buffer(const Base::Context& context, const Settings& orig_settings);

    // IObject overrides
    bool SetName(std::string_view name) override;

    // IBuffer overrides
    void SetData(Rhi::ICommandQueue& target_cmd_queue, const SubResource& sub_resource) override;
    SubResource GetData(Rhi::ICommandQueue& target_cmd_queue, const BytesRangeOpt& data_range = {}) override;
    Opt<Descriptor> InitializeNativeViewDescriptor(const View::Id& view_id) override;

    D3D12_VERTEX_BUFFER_VIEW        GetNativeVertexBufferView() const;
    D3D12_INDEX_BUFFER_VIEW         GetNativeIndexBufferView() const;
    D3D12_CONSTANT_BUFFER_VIEW_DESC GetNativeConstantBufferViewDesc() const;

private:
    wrl::ComPtr<ID3D12Resource> m_cp_upload_resource;
};

} // namespace Methane::Graphics::DirectX
