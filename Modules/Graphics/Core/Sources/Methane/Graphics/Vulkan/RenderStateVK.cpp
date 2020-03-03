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

Ptr<RenderState> RenderState::Create(RenderContext& context, const RenderState::Settings& state_settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderStateVK>(dynamic_cast<RenderContextBase&>(context), state_settings);
}

RenderStateVK::RenderStateVK(RenderContextBase& context, const Settings& settings)
    : RenderStateBase(context, settings)
{
    ITT_FUNCTION_TASK();
    Reset(settings);
}

RenderStateVK::~RenderStateVK()
{
    ITT_FUNCTION_TASK();
}

void RenderStateVK::Reset(const Settings& settings)
{
    ITT_FUNCTION_TASK();
    if (!settings.sp_program)
    {
        throw std::invalid_argument("Can not create state with empty program.");
    }

    RenderStateBase::Reset(settings);

    ProgramVK& vulkan_program = static_cast<ProgramVK&>(*settings.sp_program);

    if (!settings.viewports.empty())
    {
        SetViewports(settings.viewports);
    }
    if (!settings.scissor_rects.empty())
    {
        SetScissorRects(settings.scissor_rects);
    }
    
    ResetNativeState();
}

void RenderStateVK::Apply(RenderCommandListBase& command_list, Group::Mask state_groups)
{
    ITT_FUNCTION_TASK();
}

void RenderStateVK::SetViewports(const Viewports& viewports)
{
    ITT_FUNCTION_TASK();

    RenderStateBase::SetViewports(viewports);
}

void RenderStateVK::SetScissorRects(const ScissorRects& scissor_rects)
{
    ITT_FUNCTION_TASK();

    RenderStateBase::SetScissorRects(scissor_rects);
}

void RenderStateVK::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    RenderStateBase::SetName(name);
    
    ResetNativeState();
}

void RenderStateVK::ResetNativeState()
{
    ITT_FUNCTION_TASK();
}

IContextVK& RenderStateVK::GetContextVK() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<IContextVK&>(GetRenderContext());
}

} // namespace Methane::Graphics
