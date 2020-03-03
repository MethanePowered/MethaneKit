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

FILE: Methane/Graphics/Context.h
Methane base context interface: wraps graphics device used for GPU interaction.

******************************************************************************/

#pragma once

#include "Object.h"
#include "Types.h"

#include <Methane/Memory.hpp>

namespace Methane::Graphics
{

struct Device;
struct CommandQueue;
struct BlitCommandList;

struct Context : virtual Object
{
    enum class Type
    {
        Render,
    };

    enum class WaitFor
    {
        RenderComplete,
        FramePresented,
        ResourcesUploaded
    };

    struct Callback
    {
        virtual void OnContextReleased() = 0;
        virtual void OnContextInitialized() = 0;

        virtual ~Callback() = default;
    };

    // Context interface
    virtual Type GetType() const = 0;
    virtual void CompleteInitialization() = 0;
    virtual void WaitForGpu(WaitFor wait_for) = 0;
    virtual void Reset(Device& device) = 0;
    virtual void Reset() = 0;

    virtual void AddCallback(Callback& callback) = 0;
    virtual void RemoveCallback(Callback& callback) = 0;

    virtual Device&          GetDevice() = 0;
    virtual CommandQueue&    GetUploadCommandQueue() = 0;
    virtual BlitCommandList& GetUploadCommandList() = 0;
};

} // namespace Methane::Graphics
