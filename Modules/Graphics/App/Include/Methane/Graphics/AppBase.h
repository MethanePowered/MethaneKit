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
#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/ImageLoader.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

struct Texture;
struct ViewState;
struct RenderPass;

struct AppSettings
{
    Platform::App::Settings platform_app;
    IApp::Settings          graphics_app;
    RenderContext::Settings render_context;
};

class AppBase
    : public Platform::App
    , protected Data::Receiver<IContextCallback>
{
public:
    explicit AppBase(const AppSettings& settings, Data::Provider& textures_provider);

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
        Resource::DescriptorByUsage descriptor_by_usage;
        std::string                 name;

        ResourceRestoreInfo() = default;
        ResourceRestoreInfo(const ResourceRestoreInfo&) = default;
        explicit ResourceRestoreInfo(const Ptr<Resource>& resource_ptr)
            : descriptor_by_usage(resource_ptr ? resource_ptr->GetDescriptorByUsage() : Resource::DescriptorByUsage())
            , name(resource_ptr ? resource_ptr->GetName() : std::string())
        { }
    };

    Ptr<RenderPass> CreateScreenRenderPass(const Ptr<Texture>& frame_buffer_texture) const;
    ResourceRestoreInfo ReleaseDepthTexture();
    void RestoreDepthTexture(const ResourceRestoreInfo& depth_restore_info);

    const Graphics::IApp::Settings& GetBaseGraphicsAppSettings() const noexcept { return m_settings; }
    bool SetBaseAnimationsEnabled(bool animations_enabled);

    void UpdateWindowTitle();
    void CompleteInitialization() const;

    // Platform::AppBase interface
    Platform::AppView GetView() const override { return m_context_ptr->GetAppView(); }

    // IContextCallback implementation
    void OnContextReleased(Context&) override;
    void OnContextCompletingInitialization(Context&) override { /* no event handling logic is needed here */ }
    void OnContextInitialized(Context&) override;

    const RenderContext::Settings& GetInitialContextSettings() const noexcept  { return m_initial_context_settings; }
    bool                           IsRenderContextInitialized() const noexcept { return !!m_context_ptr; }
    const Ptr<RenderContext>&      GetRenderContextPtr() const noexcept        { return m_context_ptr; }
    RenderContext&                 GetRenderContext() const                    { META_CHECK_ARG_NOT_NULL(m_context_ptr); return *m_context_ptr; }
    const Ptr<ViewState>&          GetViewStatePtr() const noexcept            { return m_view_state_ptr; }
    ViewState&                     GetViewState()                              { META_CHECK_ARG_NOT_NULL(m_view_state_ptr); return *m_view_state_ptr; }
    FrameSize                      GetFrameSizeInDots() const noexcept         { return m_context_ptr->GetSettings().frame_size / m_context_ptr->GetContentScalingFactor(); }
    ImageLoader&                   GetImageLoader() noexcept                   { return m_image_loader; }
    Data::AnimationsPool&          GetAnimations() noexcept                    { return m_animations; }
    const Ptr<Texture>&            GetDepthTexturePtr() const noexcept         { return m_depth_texture_ptr; }

    static std::string IndexedName(const std::string& base_name, uint32_t index);

private:
    Graphics::IApp::Settings m_settings;
    RenderContext::Settings  m_initial_context_settings;
    Timer                    m_title_update_timer;
    ImageLoader              m_image_loader;
    Data::AnimationsPool     m_animations;
    Ptr<RenderContext>       m_context_ptr;
    Ptr<Texture>             m_depth_texture_ptr;
    Ptr<ViewState>           m_view_state_ptr;
    bool                     m_restore_animations_enabled = true;
};

} // namespace Methane::Graphics