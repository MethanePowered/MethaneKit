/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Windows/Primitives.cpp
Windows platform graphics primitives.

******************************************************************************/

#include <Methane/Graphics/Primitives.h>
#include <Methane/Instrumentation.h>

#include <system_error>
#include <sstream>

namespace Methane::Graphics
{

static std::string GetErrorMessage(HRESULT hr, ID3D12Device* device = nullptr)
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    if (hr == DXGI_ERROR_DEVICE_REMOVED && device)
    {
        ss << "DirectX device was removed with error: ";
        hr = device->GetDeviceRemovedReason();
    }
    else
    {
        ss << "Critical DirectX runtime error has occurred: ";
    }
    ss << std::system_category().message(hr);
    return ss.str();
}

static std::string GetErrorMessage(HRESULT hr, const wrl::ComPtr<ID3DBlob>& error_blob)
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    ss << "Critical DirectX runtime error has occurred: ";
    ss << std::system_category().message(hr);
    if (error_blob.Get())
    {
        ss << std::endl << "Error details: ";
        ss << static_cast<char*>(error_blob->GetBufferPointer());
        error_blob->Release();
    }
    return ss.str();
}

RuntimeException::RuntimeException(HRESULT hr, ID3D12Device* device)
    : std::runtime_error(GetErrorMessage(hr, device))
    , m_result(hr == DXGI_ERROR_DEVICE_REMOVED && device ? device->GetDeviceRemovedReason() : hr)
    , m_device(device)
{
    META_FUNCTION_TASK();
    META_LOG(what() + "\n");
}

RuntimeException::RuntimeException(HRESULT hr, const wrl::ComPtr<ID3DBlob>& error_blob)
    : std::runtime_error(GetErrorMessage(hr, error_blob))
    , m_result(hr)
{
    META_FUNCTION_TASK();
    META_LOG(what() + "\n");
}

} // namespace Methane::Graphics