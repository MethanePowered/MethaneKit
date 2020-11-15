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

FILE: Methane/Graphics/DirectX12/ContextDX.hpp
DirectX 12 base template implementation of the context interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ContextBase.h>
#include "FenceDX.h"
#include "DeviceDX.h"
#include "CommandQueueDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/Windows/ErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <wrl.h>
#include <d3d12.h>

#include <array>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

template<class ContextBaseT, typename = std::enable_if_t<std::is_base_of_v<ContextBase, ContextBaseT>>>
class ContextDX : public ContextBaseT
{
public:
    ContextDX(DeviceBase& device, tf::Executor& parallel_executor, const typename ContextBaseT::Settings& settings)
        : ContextBaseT(device, parallel_executor, settings)
    {
        META_FUNCTION_TASK();
    }

    // ContextBase interface

    void Release() override
    {
        META_FUNCTION_TASK();
        for(wrl::ComPtr<ID3D12QueryHeap>& cp_query_heap : m_query_heaps)
        {
            cp_query_heap.Reset();
        }
        GetMutableDeviceDX().ReleaseNativeDevice();
        ContextBaseT::Release();
        static_cast<SystemDX&>(System::Get()).ReportLiveObjects();
    }

    // Object interface

    void SetName(const std::string& name) override
    {
        META_FUNCTION_TASK();
        ContextBaseT::SetName(name);
        GetDevice().SetName(name + " Device");
    }

    // IContextDX interface

    const DeviceDX& GetDeviceDX() const noexcept final       { return static_cast<const DeviceDX&>(GetDeviceBase()); }
    CommandQueueDX& GetUploadCommandQueueDX() noexcept final { return static_cast<CommandQueueDX&>(GetUploadCommandQueue()); }

    ID3D12QueryHeap& GetNativeQueryHeap(D3D12_QUERY_HEAP_TYPE type, uint32_t max_query_count = 1U << 15U) final
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_LESS(static_cast<size_t>(type), m_query_heaps.size());
        wrl::ComPtr<ID3D12QueryHeap>& cp_query_heap = m_query_heaps[type];
        if (!cp_query_heap)
        {
            D3D12_QUERY_HEAP_DESC query_heap_desc{};
            query_heap_desc.Count = max_query_count;
            query_heap_desc.Type  = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
            ThrowIfFailed(GetDeviceDX().GetNativeDevice()->CreateQueryHeap(&query_heap_desc, IID_PPV_ARGS(&cp_query_heap)),
                          GetDeviceDX().GetNativeDevice().Get());
        }
        META_CHECK_ARG_NOT_NULL(cp_query_heap);
        return *cp_query_heap.Get();
    }

protected:
    DeviceDX& GetMutableDeviceDX() noexcept { return static_cast<DeviceDX&>(GetDeviceBase()); }

private:
    using NativeQueryHeaps = std::array<wrl::ComPtr<ID3D12QueryHeap>, D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP + 1>;
    NativeQueryHeaps m_query_heaps;
};

} // namespace Methane::Graphics
