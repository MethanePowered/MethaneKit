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

FILE: Methane/Graphics/Vulkan/RenderState.h
Vulkan implementation of the render state interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/IViewState.h>
#include <Methane/Graphics/Base/RenderState.h>

#include <vulkan/vulkan.hpp>

#include <map>

namespace Methane::Graphics::Rhi
{
enum class RenderPrimitive;
}

namespace Methane::Graphics::Base
{
struct RenderDrawingState;
}

namespace Methane::Graphics::Vulkan
{

struct IContext;
class ViewState;

class RenderState final
    : public Base::RenderState
{
public:
    static vk::PrimitiveTopology GetVulkanPrimitiveTopology(Rhi::RenderPrimitive primitive_type);

    RenderState(const Base::RenderContext& context, const Settings& settings);
    
    // IRenderState interface
    void Reset(const Settings& settings) override;

    // Base::RenderState interface
    void Apply(Base::RenderCommandList& render_command_list, Groups state_groups) override;

    // IObject interface
    bool SetName(std::string_view name) override;

    bool                IsNativePipelineDynamic() const noexcept  { return !Base::RenderState::IsDeferred(); }
    const vk::Pipeline& GetNativePipelineDynamic() const;
    const vk::Pipeline& GetNativePipelineMonolithic(const ViewState& viewState, Rhi::RenderPrimitive renderPrimitive);
    const vk::Pipeline& GetNativePipelineMonolithic(const Base::RenderDrawingState& drawing_state);

private:
    vk::UniquePipeline CreateNativePipeline(const ViewState* viewState = nullptr, Opt<Rhi::RenderPrimitive> renderPrimitive = {});

    using PipelineId = std::tuple<Rhi::ViewSettings, Rhi::RenderPrimitive>;
    using MonolithicPipelineById = std::map<PipelineId, vk::UniquePipeline>;

    const IContext&        m_vk_context;
    vk::UniquePipeline     m_vk_pipeline_dynamic;
    MonolithicPipelineById m_vk_pipeline_monolithic_by_id;
};

} // namespace Methane::Graphics::Vulkan
