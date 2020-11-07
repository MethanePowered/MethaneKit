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

FILE: Methane/Graphics/Windows/Primitives.h
Windows platform graphics primitives.

******************************************************************************/

#pragma once

#include <wrl.h>
#include <d3dcommon.h>
#include <d3d12.h>

#include <string>
#include <stdexcept>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

class RuntimeException : public std::runtime_error
{
public:
    RuntimeException(HRESULT hr, ID3D12Device* device = nullptr);
    RuntimeException(HRESULT hr, const wrl::ComPtr<ID3DBlob>& error_blob);

    HRESULT             GetResult() const noexcept { return m_result; }
    const ID3D12Device* GetDevice() const noexcept { return m_device; }

private:
    const HRESULT       m_result;
    const ID3D12Device* m_device = nullptr;
};

inline void SafeCloseHandle(HANDLE& handle) noexcept
{
    if (!handle)
        return;

    CloseHandle(handle);
    handle = nullptr;
}

inline void ThrowIfFailed(HRESULT hr, ID3D12Device* p_device = nullptr)
{
    if (FAILED(hr))
        throw RuntimeException(hr, p_device);
}

inline void ThrowIfFailed(HRESULT hr, const wrl::ComPtr<ID3DBlob>& error_blob)
{
    if (FAILED(hr))
        throw RuntimeException(hr, error_blob);
}

} // namespace Methane::Graphics
