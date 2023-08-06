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

FILE: Methane/Graphics/AppBase.h
Base implementation of the Methane graphics application.

******************************************************************************/

#pragma once

#include "IApp.h"
#include "CombinedAppSettings.h"

#include <Methane/Data/IProvider.h>
#include <Methane/Data/AnimationsPool.h>
#include <Methane/Data/Receiver.hpp>
#include <Methane/Platform/App.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/RenderPass.h>
#include <Methane/Graphics/RHI/RenderPattern.h>
#include <Methane/Graphics/RHI/ViewState.h>
#include <Methane/Graphics/ImageLoader.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

class AppBase // NOSONAR
    : public Platform::App
    , protected Data::Receiver<Rhi::IContextCallback> //NOSONAR
{
public:
    AppBase(const CombinedAppSettings& settings, Data::IProvider& textures_provider);
    ~AppBase() override;

    AppBase(const AppBase&) = delete;
    AppBase(AppBase&&) = delete;

    AppBase& operator=(const AppBase&) = delete;
    AppBase& operator=(AppBase&&) = delete;

    // Platform::App interface
    void InitContext(const Platform::AppEnvironment& env, const FrameSize& frame_size) override;
    void Init() override;
    void StartResizing() override;
    void EndResizing() override;
    bool Resize(const FrameSize& frame_size, bool is_minimized) override;
    bool Update() override;
    bool Render() override;
    bool SetFullScreen(bool is_full_screen) override;

    void SetShowHudInWindowTitle(bool show_hud_in_window_title);

protected:
    struct ResourceRestoreInfo
    {
        Rhi::IResource::DescriptorByViewId descriptor_by_view_id;
        std::string name;

        explicit ResourceRestoreInfo(const Rhi::IResource& resource);
        ResourceRestoreInfo(const ResourceRestoreInfo& other) = default;
        ResourceRestoreInfo(ResourceRestoreInfo&& other) noexcept = default;

        ResourceRestoreInfo& operator=(const ResourceRestoreInfo& other) = default;
        ResourceRestoreInfo& operator=(ResourceRestoreInfo&& other) noexcept = default;
    };

    Rhi::Device GetDefaultDevice() const;
    Rhi::TextureViews GetScreenPassAttachments(const Rhi::Texture& frame_buffer_texture) const;
    Rhi::RenderPass   CreateScreenRenderPass(const Rhi::Texture& frame_buffer_texture) const;
    Opt<ResourceRestoreInfo> ReleaseDepthTexture();
    void RestoreDepthTexture(const Opt<ResourceRestoreInfo>& depth_restore_info_opt);

    const Graphics::IApp::Settings& GetBaseGraphicsAppSettings() const noexcept { return m_settings; }
    bool SetBaseAnimationsEnabled(bool animations_enabled);

    void UpdateWindowTitle();
    void CompleteInitialization() const;
    void WaitForRenderComplete() const;

    // Platform::AppBase interface
    Platform::AppView GetView() const override { return m_context.GetAppView(); }

    // IContextCallback implementation
    void OnContextReleased(Rhi::IContext&) override;
    void OnContextCompletingInitialization(Rhi::IContext&) override { /* no event handling logic is needed here */ }
    void OnContextInitialized(Rhi::IContext&) override;

    const Rhi::RenderContextSettings& GetInitialContextSettings() const noexcept  { return m_initial_context_settings; }
    Rhi::IRenderPattern::Settings&    GetScreenRenderPatternSettings() noexcept   { return m_screen_pass_pattern_settings; }
    const Rhi::RenderContext&         GetRenderContext() const noexcept           { return m_context; }
    const Rhi::RenderPattern&         GetScreenRenderPattern() const noexcept     { return m_screen_render_pattern; }
    const Rhi::ViewState&             GetViewState() const noexcept               { return m_view_state; }
    const Rhi::Texture&               GetDepthTexture() const noexcept            { return m_depth_texture; }
    FrameSize                         GetFrameSizeInDots() const                  { return m_context.GetSettings().frame_size / GetContentScalingFactor(); }
    ImageLoader&                      GetImageLoader() noexcept                   { return m_image_loader; }
    Data::AnimationsPool&             GetAnimations() noexcept                    { return m_animations; }

private:
    Graphics::IApp::Settings   m_settings;
    Rhi::RenderContextSettings m_initial_context_settings;
    Rhi::RenderPatternSettings m_screen_pass_pattern_settings;
    Timer                      m_title_update_timer;
    ImageLoader                m_image_loader;
    Data::AnimationsPool       m_animations;
    Rhi::RenderContext         m_context;
    Rhi::Texture               m_depth_texture;
    Rhi::RenderPattern         m_screen_render_pattern;
    Rhi::ViewState             m_view_state;
    bool                       m_restore_animations_enabled = true;
};

} // namespace Methane::Graphics