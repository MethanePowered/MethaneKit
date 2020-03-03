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

FILE: Methane/Graphics/DirectX12/CommandListDX.h
DirectX 12 command list accessor interface for template class CommandListDX<CommandListBaseT>

******************************************************************************/

#pragma once

#include <wrl.h>
#include <d3d12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

class CommandQueueDX;

struct ICommandListDX
{
    virtual CommandQueueDX&             GetCommandQueueDX() = 0;
    virtual ID3D12GraphicsCommandList&  GetNativeCommandList() const = 0;
    virtual ID3D12GraphicsCommandList4* GetNativeCommandList4() const = 0;

    virtual ~ICommandListDX() = default;
};

} // namespace Methane::Graphics
