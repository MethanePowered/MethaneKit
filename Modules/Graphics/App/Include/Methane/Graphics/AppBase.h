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

#include "App.h"

#include <Methane/Data/Provider.h>
#include <Methane/Data/AnimationsPool.h>
#include <Methane/Data/Receiver.hpp>
#include <Methane/Platform/App.h>
#include <Methane/Graphics/IRenderContext.h>
#include <Methane/Graphics/ImageLoader.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

struct ITexture;
struct IViewState;
struct RenderPass;

struct AppSettings
{
    Platform::IApp::Settings platform_app;
    Graphics::IApp::Settings graphics_app;
    RenderContextSettings render_context;

    AppSettings& SetPlatformAppSettings(Platform::IApp::Settings&& new_platform_app_settings) noexcept;
    AppSettings& SetGraphicsAppSettings(Graphics::IApp::Settings&& new_graphics_app_settings) noexcept;
    AppSettings& SetRenderContextSettings(RenderContextSettings&& new_render_context_settings) noexcept;
};

class AppBase // NOSONAR
    : public Platform::App
    , protected Data::Receiver<IContextCallback> //NOSONAR
{
public:
    AppBase(const AppSettings& settings, Data::Provider& textures_provider);
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
        IResource::DescriptorByViewId descriptor_by_view_id;
        std::string                   name;

        explicit ResourceRestoreInfo(const IResource& resource);
        ResourceRestoreInfo(const ResourceRestoreInfo& other) = default;
        ResourceRestoreInfo(ResourceRestoreInfo&& other) noexcept = default;

        ResourceRestoreInfo& operator=(const ResourceRestoreInfo& other) = default;
        ResourceRestoreInfo& operator=(ResourceRestoreInfo&& other) noexcept = default;
    };

    ITexture::Views GetScreenPassAttachments(ITexture& frame_buffer_texture) const;
    Ptr<RenderPass> CreateScreenRenderPass(ITexture& frame_buffer_texture) const;
    Opt<ResourceRestoreInfo> ReleaseDepthTexture();
    void RestoreDepthTexture(const Opt<ResourceRestoreInfo>& depth_restore_info_opt);

    const Graphics::IApp::Settings& GetBaseGraphicsAppSettings() const noexcept { return m_settings; }
    bool SetBaseAnimationsEnabled(bool animations_enabled);

    void UpdateWindowTitle();
    void CompleteInitialization() const;
    void WaitForRenderComplete() const;

    // Platform::AppBase interface
    Platform::AppView GetView() const override { return m_context_ptr->GetAppView(); }

    // IContextCallback implementation
    void OnContextReleased(IContext&) override;
    void OnContextCompletingInitialization(IContext&) override { /* no event handling logic is needed here */ }
    void OnContextInitialized(IContext&) override;

    const RenderContextSettings&  GetInitialContextSettings() const noexcept    { return m_initial_context_settings; }
    RenderPattern::Settings&        GetScreenRenderPatternSettings() noexcept     { return m_screen_pass_pattern_settings; }
    bool                            IsRenderContextInitialized() const noexcept   { return !!m_context_ptr; }
    const Ptr<IRenderContext>&       GetRenderContextPtr() const noexcept          { return m_context_ptr; }
    IRenderContext&                  GetRenderContext() const                      { META_CHECK_ARG_NOT_NULL(m_context_ptr); return *m_context_ptr; }
    const Ptr<RenderPattern>&       GetScreenRenderPatternPtr() const noexcept    { return m_screen_render_pattern_ptr; }
    RenderPattern&                  GetScreenRenderPattern() const                { META_CHECK_ARG_NOT_NULL(m_screen_render_pattern_ptr); return *m_screen_render_pattern_ptr; }
    const Ptr<IViewState>&          GetViewStatePtr() const noexcept              { return m_view_state_ptr; }
    IViewState&                     GetViewState()                                { META_CHECK_ARG_NOT_NULL(m_view_state_ptr); return *m_view_state_ptr; }
    FrameSize                       GetFrameSizeInDots() const                    { return m_context_ptr->GetSettings().frame_size / GetContentScalingFactor(); }
    ImageLoader&                    GetImageLoader() noexcept                     { return m_image_loader; }
    Data::AnimationsPool&           GetAnimations() noexcept                      { return m_animations; }
    const Ptr<ITexture>&            GetDepthTexturePtr() const noexcept           { return m_depth_texture_ptr; }
    ITexture&                       GetDepthTexture() const                       { META_CHECK_ARG_NOT_NULL(m_depth_texture_ptr); return *m_depth_texture_ptr; }

    static std::string IndexedName(const std::string& base_name, uint32_t index);

private:
    Graphics::IApp::Settings m_settings;
    RenderContextSettings    m_initial_context_settings;
    RenderPattern::Settings  m_screen_pass_pattern_settings;
    Timer                    m_title_update_timer;
    ImageLoader              m_image_loader;
    Data::AnimationsPool     m_animations;
    Ptr<IRenderContext>      m_context_ptr;
    Ptr<ITexture>            m_depth_texture_ptr;
    Ptr<RenderPattern>       m_screen_render_pattern_ptr;
    Ptr<IViewState>          m_view_state_ptr;
    bool                     m_restore_animations_enabled = true;
};

} // namespace Methane::Graphics