/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX12/ReleasePoolDX.h
DirectX 12 GPU release pool for deferred objects release.

******************************************************************************/
#pragma once

#include <Methane/Graphics/ReleasePool.h>

#include <wrl.h>
#include <d3d12.h>

#include <vector>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

class ReleasePoolDX final : public ReleasePool
{
public:
    ReleasePoolDX() = default;

    void AddResource(ResourceBase& resource) override;
    void ReleaseAllResources() override;
    void ReleaseFrameResources(uint32_t frame_index) override;

private:
    using D3DResourceComPtrs = std::vector<wrl::ComPtr<ID3D12Resource>>;

    std::vector<D3DResourceComPtrs> m_frame_resources;
    D3DResourceComPtrs              m_misc_resources;
};

}