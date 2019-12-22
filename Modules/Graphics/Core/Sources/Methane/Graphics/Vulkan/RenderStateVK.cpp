/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

#include <Methane/Data/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

RenderState::Ptr RenderState::Create(Context& context, const RenderState::Settings& state_settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderStateVK>(static_cast<ContextBase&>(context), state_settings);
}

RenderStateVK::RenderStateVK(ContextBase& context, const Settings& settings)
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
    
    ProgramVK& vulkan_program = static_cast<ProgramVK&>(*m_settings.sp_program);

    if (!m_settings.viewports.empty())
    {
        SetViewports(m_settings.viewports);
    }
    if (!m_settings.scissor_rects.empty())
    {
        SetScissorRects(m_settings.scissor_rects);
    }
    
    ResetNativeState();
}

void RenderStateVK::Apply(RenderCommandListBase& command_list)
{
    ITT_FUNCTION_TASK();

    RenderCommandListVK& vulkan_command_list = static_cast<RenderCommandListVK&>(command_list);
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

ContextVK& RenderStateVK::GetContextVK() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextVK&>(m_context);
}

} // namespace Methane::Graphics
