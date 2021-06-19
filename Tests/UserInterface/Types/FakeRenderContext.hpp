/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Tests/UserInterface/Types/FakeRenderContext.hpp
Fake render context used for UI types testing

******************************************************************************/

#pragma once

#include <Methane/Exceptions.hpp>
#include <Methane/Graphics/Device.h>
#include <Methane/Graphics/BlitCommandList.h>
#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/CommandQueue.h>
#include <Methane/Graphics/FpsCounter.h>
#include <Methane/Data/Emitter.hpp>

namespace Methane::Graphics
{

class FakeObjectRegistry : public Object::Registry
{
public:
    void                      AddGraphicsObject(Object&) override                           { META_FUNCTION_NOT_IMPLEMENTED(); }
    [[nodiscard]] Ptr<Object> GetGraphicsObject(const std::string&) const noexcept override { return nullptr; }
    [[nodiscard]] bool        HasGraphicsObject(const std::string&) const noexcept override { return false; }
};

class FakeDevice : public Device, public Data::Emitter<IDeviceCallback>
{
public:
    // Device interface
    [[nodiscard]] const std::string& GetAdapterName() const noexcept override { static std::string s_name; return s_name; }
    [[nodiscard]] bool IsSoftwareAdapter() const noexcept override { return true; }
    [[nodiscard]] const Capabilities& GetCapabilities() const noexcept override { static const Capabilities s_caps; return s_caps; }
    [[nodiscard]] std::string ToString() const override { return { }; }

    // Object interface
    void SetName(const std::string&) override                          { META_FUNCTION_NOT_IMPLEMENTED(); }
    [[nodiscard]] const std::string& GetName() const noexcept override { static std::string name; return name; }
    [[nodiscard]] Ptr<Object>        GetPtr() override                 { return nullptr; }
};

class FakeCommandQueue : public CommandQueue
{
public:
    FakeCommandQueue(const Context& context, CommandList::Type type)
        : m_context(context)
        , m_type(type)
    { }

    // CommandQueue interface
    [[nodiscard]] const Context&    GetContext() const noexcept override          { return m_context; }
    [[nodiscard]] CommandList::Type GetCommandListType() const noexcept override  { return m_type; }
    void Execute(CommandListSet&, const CommandList::CompletedCallback&) override { META_FUNCTION_NOT_IMPLEMENTED(); }

    // Object interface
    void SetName(const std::string&) override                          { META_FUNCTION_NOT_IMPLEMENTED(); }
    [[nodiscard]] const std::string& GetName() const noexcept override { static std::string name; return name; }
    [[nodiscard]] Ptr<Object>        GetPtr() override                 { return nullptr; }

private:
    const Context&    m_context;
    CommandList::Type m_type;
};

class FakeCommandListSet : public CommandListSet
{
public:
    [[nodiscard]] Data::Size               GetCount() const noexcept override       { return 0; }
    [[nodiscard]] const Refs<CommandList>& GetRefs() const noexcept override        { return m_command_list_refs; }
    [[nodiscard]] CommandList&             operator[](Data::Index) const override   { META_FUNCTION_NOT_IMPLEMENTED(); }

private:
    Refs<CommandList> m_command_list_refs;
};

template<typename CommandListType, CommandList::Type command_list_type>
class FakeCommandList : public CommandListType
{
public:
    FakeCommandList(CommandQueue& command_queue)
        : m_command_queue(command_queue)
    { }

    // CommandList interface
    [[nodiscard]] CommandList::Type  GetType() const noexcept override                  { return command_list_type; }
    [[nodiscard]] CommandList::State GetState() const noexcept override                 { return CommandList::State::Pending; }
    void PopDebugGroup() override                                                       { META_FUNCTION_NOT_IMPLEMENTED(); }
    void PushDebugGroup(CommandList::DebugGroup&) override                              { META_FUNCTION_NOT_IMPLEMENTED(); }
    void Reset(CommandList::DebugGroup*) override                                       { META_FUNCTION_NOT_IMPLEMENTED(); }
    void ResetOnce(CommandList::DebugGroup*) override                                   { META_FUNCTION_NOT_IMPLEMENTED(); }
    void SetProgramBindings(ProgramBindings&, ProgramBindings::ApplyBehavior) override  { META_FUNCTION_NOT_IMPLEMENTED(); }
    void SetResourceBarriers(const Resource::Barriers&) override                        { META_FUNCTION_NOT_IMPLEMENTED(); }
    void Commit() override                                                              { META_FUNCTION_NOT_IMPLEMENTED(); }
    void WaitUntilCompleted(uint32_t) override                                          { META_FUNCTION_NOT_IMPLEMENTED(); }
    [[nodiscard]] Data::TimeRange GetGpuTimeRange(bool) const override                  { throw Data::TimeRange{ }; }
    [[nodiscard]] CommandQueue& GetCommandQueue() override                              { return m_command_queue; }

    // Object interface
    void SetName(const std::string&) override                          { META_FUNCTION_NOT_IMPLEMENTED(); }
    [[nodiscard]] const std::string& GetName() const noexcept override { static std::string name; return name; }
    [[nodiscard]] Ptr<Object>        GetPtr() override                 { return nullptr; }

private:
    CommandQueue& m_command_queue;
};

using FakeBlitCommandList   = FakeCommandList<BlitCommandList,   CommandList::Type::Blit>;

class FakeRenderContext
    : public RenderContext
    , public Data::Emitter<IContextCallback>
{
public:
    FakeRenderContext(const Settings& settings, float content_scale, uint32_t font_dpi)
        : m_settings(settings)
        , m_content_scale(content_scale)
        , m_font_dpi(font_dpi)
    { }

    // RenderContext interface
    [[nodiscard]] bool ReadyToRender() const override                                   { return false; }
    void Resize(const FrameSize&) override                                              { throw Methane::NotImplementedException("Resize"); }
    void Present() override                                                             { throw Methane::NotImplementedException("Present"); }
    [[nodiscard]] Platform::AppView GetAppView() const override                         { return { }; }
    [[nodiscard]] const Settings&   GetSettings() const noexcept override               { return m_settings; }
    [[nodiscard]] uint32_t          GetFrameBufferIndex() const noexcept override       { return 0U; }
    [[nodiscard]] uint32_t          GetFrameIndex() const noexcept override             { return 0U; }
    [[nodiscard]] float             GetContentScalingFactor() const override            { return m_content_scale; }
    [[nodiscard]] uint32_t          GetFontResolutionDpi() const override               { return m_font_dpi; }
    [[nodiscard]] const FpsCounter& GetFpsCounter() const noexcept override             { return m_fps_counter; }
    bool SetVSyncEnabled(bool vsync_enabled) override                                   { m_settings.vsync_enabled = vsync_enabled; return true; }
    bool SetFrameBuffersCount(uint32_t frame_buffers_count) override                    { m_settings.frame_buffers_count = frame_buffers_count; return true; }
    bool SetFullScreen(bool is_full_screen) override                                    { m_settings.is_full_screen = is_full_screen; return true; }

    // Context interface
    [[nodiscard]] Type GetType() const noexcept override                                { return Type::Render; }
    [[nodiscard]] Options GetOptions() const noexcept override                          { return Options::None; }
    [[nodiscard]] tf::Executor& GetParallelExecutor() const noexcept override           { return m_executor; }
    [[nodiscard]] Object::Registry& GetObjectsRegistry() noexcept override              { return m_object_registry; }
    void RequestDeferredAction(DeferredAction) const noexcept override                  { }
    void CompleteInitialization() override                                              { throw Methane::NotImplementedException("CompleteInitialization"); }
    [[nodiscard]] bool IsCompletingInitialization() const noexcept override             { return false; }
    void WaitForGpu(WaitFor) override                                                   { throw Methane::NotImplementedException("WaitForGpu"); }
    void Reset(Device&) override                                                        { throw Methane::NotImplementedException("Reset"); }
    void Reset() override                                                               { throw Methane::NotImplementedException("Reset"); }

    [[nodiscard]] const Device& GetDevice() const override                              { return m_fake_device; }
    [[nodiscard]] CommandKit& GetDefaultCommandKit(CommandList::Type) const override    { throw Methane::NotImplementedException("GetDefaultCommandKit"); }
    [[nodiscard]] CommandKit& GetDefaultCommandKit(CommandQueue&) const override        { throw Methane::NotImplementedException("GetDefaultCommandKit"); }

    // Object interface
    void SetName(const std::string&) override                                           { throw Methane::NotImplementedException("SetName"); }
    [[nodiscard]] const std::string& GetName() const noexcept override                  { static std::string name; return name; }
    [[nodiscard]] Ptr<Object>        GetPtr() override                                  { return nullptr; }

private:
    Settings               m_settings;
    float                  m_content_scale;
    uint32_t               m_font_dpi;
    FakeDevice             m_fake_device;
    FpsCounter             m_fps_counter;
    FakeObjectRegistry     m_object_registry;
    mutable tf::Executor   m_executor;
};

} // namespace Methane::Graphics