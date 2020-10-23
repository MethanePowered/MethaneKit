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

FILE: Methane/Graphics/Vulkan/RenderStateVK.mm
Vulkan implementation of the render state interface.

******************************************************************************/

#include "RenderStateVK.h"
#include "ContextVK.h"
#include "DeviceVK.h"
#include "RenderCommandListVK.h"
#include "ProgramVK.h"
#include "ShaderVK.h"
#include "TypesVK.h"

#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<ViewState> ViewState::Create(const ViewState::Settings& state_settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<ViewStateVK>(state_settings);
}

ViewStateVK::ViewStateVK(const Settings& settings)
    : ViewStateBase(settings)
{
    META_FUNCTION_TASK();
}

bool ViewStateVK::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (!ViewStateBase::Reset(settings))
        return false;

    return true;
}

bool ViewStateVK::SetViewports(const Viewports& viewports)
{
    META_FUNCTION_TASK();
    if (!ViewStateBase::SetViewports(viewports))
        return false;

    return true;
}

bool ViewStateVK::SetScissorRects(const ScissorRects& scissor_rects)
{
    META_FUNCTION_TASK();
    if (!ViewStateBase::SetScissorRects(scissor_rects))
        return false;

    return true;
}

void ViewStateVK::Apply(RenderCommandListBase& command_list)
{
    META_FUNCTION_TASK();

    RenderCommandListVK& vulkan_command_list = static_cast<RenderCommandListVK&>(command_list);
    META_UNUSED(vulkan_command_list);
}

Ptr<RenderState> RenderState::Create(RenderContext& context, const RenderState::Settings& state_settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderStateVK>(dynamic_cast<RenderContextBase&>(context), state_settings);
}

RenderStateVK::RenderStateVK(RenderContextBase& context, const Settings& settings)
    : RenderStateBase(context, settings)
{
    META_FUNCTION_TASK();
    Reset(settings);
}

RenderStateVK::~RenderStateVK()
{
    META_FUNCTION_TASK();
}

void RenderStateVK::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (!settings.program_ptr)
    {
        throw std::invalid_argument("Can not create state with empty program.");
    }

    RenderStateBase::Reset(settings);
    ResetNativeState();
}

void RenderStateVK::Apply(RenderCommandListBase& /*command_list*/, Group::Mask /*state_groups*/)
{
    META_FUNCTION_TASK();
}

void RenderStateVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();

    RenderStateBase::SetName(name);
    
    ResetNativeState();
}

void RenderStateVK::ResetNativeState()
{
    META_FUNCTION_TASK();
}

IContextVK& RenderStateVK::GetContextVK() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<IContextVK&>(GetRenderContext());
}

} // namespace Methane::Graphics
