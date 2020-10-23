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

FILE: Methane/Graphics/Windows/Helpers.h
Windows platform graphics helpers.

******************************************************************************/

#pragma once

#include <wrl.h>
#include <d3dcommon.h>
#include <d3d12.h>

#include <string>
#include <stdexcept>
#include <system_error>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

inline void SafeCloseHandle(HANDLE& handle)
{
    META_FUNCTION_TASK();
    if (!handle)
        return;

    CloseHandle(handle);
    handle = nullptr;
}

inline void ThrowIfFailed(HRESULT hr, ID3D12Device* p_device = nullptr)
{
    if (!FAILED(hr))
        return;

    std::string error_msg;
    if (hr == DXGI_ERROR_DEVICE_REMOVED && p_device)
    {
        error_msg = "DirectX device was removed with error: ";
        hr = p_device->GetDeviceRemovedReason();
    }
    else
    {
        error_msg = "Critical DirectX runtime error has occurred: ";
    }
    error_msg += std::system_category().message(hr);

    META_LOG(error_msg + "\n");
    throw std::runtime_error(error_msg);
}

inline void ThrowIfFailed(HRESULT hr, wrl::ComPtr<ID3DBlob>& error_blob)
{
    if (!FAILED(hr))
        return;

    std::string error_msg = "Critical DirectX runtime error has occurred: ";
    error_msg += std::system_category().message(hr);
    if (error_blob.Get())
    {
        error_msg += "\nError details: ";
        error_msg += static_cast<char*>(error_blob->GetBufferPointer());
        error_blob->Release();
    }

    META_LOG(error_msg + "\n");
    throw std::runtime_error(error_msg);
}

} // namespace Methane::Graphics
