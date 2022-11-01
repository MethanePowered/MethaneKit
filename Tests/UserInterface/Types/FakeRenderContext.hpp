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
#include <Methane/Graphics/IDevice.h>
#include <Methane/Graphics/IRenderPass.h>
#include <Methane/Graphics/ITransferCommandList.h>
#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/IRenderContext.h>
#include <Methane/Graphics/ICommandQueue.h>
#include <Methane/Graphics/FpsCounter.h>
#include <Methane/Data/Emitter.hpp>

namespace Methane::Graphics
{

class FakeObjectRegistry
    : public IObjectRegistry
{
public:
    void                       AddGraphicsObject(IObject&) override                          { META_FUNCTION_NOT_IMPLEMENTED(); }
    void                       RemoveGraphicsObject(IObject&) override                       { META_FUNCTION_NOT_IMPLEMENTED(); }
    [[nodiscard]] Ptr<IObject> GetGraphicsObject(const std::string&) const noexcept override { return nullptr; }
    [[nodiscard]] bool         HasGraphicsObject(const std::string&) const noexcept override { return false; }
};

class FakeDevice
    : public IDevice
    , public Data::Emitter<IDeviceCallback>
    , public Data::Emitter<IObjectCallback>
    , public std::enable_shared_from_this<FakeDevice>
{
public:
    // IDevice interface
    [[nodiscard]] const std::string& GetAdapterName() const noexcept override { static std::string s_name; return s_name; }
    [[nodiscard]] bool IsSoftwareAdapter() const noexcept override { return true; }
    [[nodiscard]] const Capabilities& GetCapabilities() const noexcept override { static const Capabilities s_caps; return s_caps; }
    [[nodiscard]] std::string ToString() const override { return { }; }

    // IObject interface
    bool SetName(const std::string&) override                          { META_FUNCTION_NOT_IMPLEMENTED_RETURN(false); }
    [[nodiscard]] const std::string& GetName() const noexcept override { static std::string name; return name; }
    [[nodiscard]] Ptr<IObject>       GetPtr() override                 { return shared_from_this(); }
};

class FakeCommandQueue
    : public ICommandQueue
    , public Data::Emitter<IObjectCallback>
    , public std::enable_shared_from_this<FakeCommandQueue>
{
public:
    FakeCommandQueue(const IContext& context, CommandListType type)
        : m_context(context)
        , m_type(type)
    { }

    // ICommandQueue interface
    [[nodiscard]] const IContext&   GetContext() const noexcept override          { return m_context; }
    [[nodiscard]] CommandListType   GetCommandListType() const noexcept override  { return m_type; }
    [[nodiscard]] uint32_t          GetFamilyIndex() const noexcept override      { return 0U; }
    void Execute(ICommandListSet&, const ICommandList::CompletedCallback&) override { META_FUNCTION_NOT_IMPLEMENTED(); }

    // IObject interface
    bool SetName(const std::string&) override                          { META_FUNCTION_NOT_IMPLEMENTED_RETURN(false); }
    [[nodiscard]] const std::string& GetName() const noexcept override { static std::string name; return name; }
    [[nodiscard]] Ptr<IObject>       GetPtr() override                 { return shared_from_this(); }

private:
    const IContext&   m_context;
    CommandListType   m_type;
};

class FakeCommandListSet
    : public ICommandListSet
{
public:
    [[nodiscard]] Data::Size                GetCount() const noexcept override       { return 0; }
    [[nodiscard]] const Refs<ICommandList>& GetRefs() const noexcept override        { return m_command_list_refs; }
    [[nodiscard]] ICommandList&             operator[](Data::Index) const override   { META_FUNCTION_NOT_IMPLEMENTED(); }

private:
    Refs<ICommandList> m_command_list_refs;
};

template<typename CommandListT, CommandListType command_list_type>
class FakeCommandList
    : public CommandListT
    , public std::enable_shared_from_this<FakeCommandList<CommandListT, command_list_type>>
{
public:
    FakeCommandList(ICommandQueue& command_queue)
        : m_command_queue(command_queue)
    { }

    // ICommandList interface
    [[nodiscard]] CommandListType  GetType() const noexcept override                     { return command_list_type; }
    [[nodiscard]] CommandListState GetState() const noexcept override                    { return CommandListState::Pending; }
    void PopDebugGroup() override                                                        { META_FUNCTION_NOT_IMPLEMENTED(); }
    void PushDebugGroup(ICommandListDebugGroup&) override                                { META_FUNCTION_NOT_IMPLEMENTED(); }
    void Reset(ICommandListDebugGroup*) override                                         { META_FUNCTION_NOT_IMPLEMENTED(); }
    void ResetOnce(ICommandListDebugGroup*) override                                     { META_FUNCTION_NOT_IMPLEMENTED(); }
    void SetProgramBindings(IProgramBindings&, IProgramBindings::ApplyBehavior) override { META_FUNCTION_NOT_IMPLEMENTED(); }
    void SetResourceBarriers(const IResourceBarriers&) override                          { META_FUNCTION_NOT_IMPLEMENTED(); }
    void Commit() override                                                               { META_FUNCTION_NOT_IMPLEMENTED(); }
    void WaitUntilCompleted(uint32_t) override                                           { META_FUNCTION_NOT_IMPLEMENTED(); }
    [[nodiscard]] Data::TimeRange GetGpuTimeRange(bool) const override                   { throw Data::TimeRange{ }; }
    [[nodiscard]] ICommandQueue& GetCommandQueue() override                               { return m_command_queue; }

    // IObject interface
    bool SetName(const std::string&) override                          { META_FUNCTION_NOT_IMPLEMENTED_RETURN(false); }
    [[nodiscard]] const std::string& GetName() const noexcept override { static std::string name; return name; }
    [[nodiscard]] Ptr<IObject>       GetPtr() override                 { return std::enable_shared_from_this<FakeCommandList<CommandListT, command_list_type>>::shared_from_this(); }

private:
    ICommandQueue& m_command_queue;
};

using FakeTransferCommandList = FakeCommandList<ITransferCommandList, CommandListType::Transfer>;

class FakeRenderContext
    : public IRenderContext
    , public Data::Emitter<IContextCallback>
    , public Data::Emitter<IObjectCallback>
    , public std::enable_shared_from_this<FakeRenderContext>
{
public:
    FakeRenderContext(const Settings& settings)
        : m_settings(settings)
    { }

    // IRenderContext interface
    [[nodiscard]] bool ReadyToRender() const override                             { return false; }
    void Resize(const FrameSize&) override                                        { META_FUNCTION_NOT_IMPLEMENTED(); }
    void Present() override                                                       { META_FUNCTION_NOT_IMPLEMENTED(); }
    [[nodiscard]] Platform::AppView GetAppView() const override                   { return { }; }
    [[nodiscard]] const Settings&   GetSettings() const noexcept override         { return m_settings; }
    [[nodiscard]] uint32_t          GetFrameBufferIndex() const noexcept override { return 0U; }
    [[nodiscard]] uint32_t          GetFrameIndex() const noexcept override       { return 0U; }
    [[nodiscard]] const FpsCounter& GetFpsCounter() const noexcept override       { return m_fps_counter; }
    bool SetVSyncEnabled(bool vsync_enabled) override                             { m_settings.vsync_enabled = vsync_enabled; return true; }
    bool SetFrameBuffersCount(uint32_t frame_buffers_count) override              { m_settings.frame_buffers_count = frame_buffers_count; return true; }
    bool SetFullScreen(bool is_full_screen) override                              { m_settings.is_full_screen = is_full_screen; return true; }

    // IContext interface
    [[nodiscard]] Type GetType() const noexcept override                                { return Type::Render; }
    [[nodiscard]] Options GetOptions() const noexcept override                          { return Options::None; }
    [[nodiscard]] tf::Executor& GetParallelExecutor() const noexcept override           { return m_executor; }
    [[nodiscard]] IObjectRegistry& GetObjectRegistry() noexcept override                { return m_object_registry; }
    [[nodiscard]] const IObjectRegistry& GetObjectRegistry() const noexcept override    { return m_object_registry; }
    void RequestDeferredAction(DeferredAction) const noexcept override                  { }
    void CompleteInitialization() override                                              { META_FUNCTION_NOT_IMPLEMENTED(); }
    [[nodiscard]] bool IsCompletingInitialization() const noexcept override             { return false; }
    void WaitForGpu(WaitFor) override                                                   { META_FUNCTION_NOT_IMPLEMENTED(); }
    void Reset(IDevice&) override                                                       { META_FUNCTION_NOT_IMPLEMENTED(); }
    void Reset() override                                                               { META_FUNCTION_NOT_IMPLEMENTED(); }

    [[nodiscard]] const IDevice& GetDevice() const override                             { return m_fake_device; }
    [[nodiscard]] ICommandKit& GetDefaultCommandKit(CommandListType) const override     { throw Methane::NotImplementedException("GetDefaultCommandKit"); }
    [[nodiscard]] ICommandKit& GetDefaultCommandKit(ICommandQueue&) const override      { throw Methane::NotImplementedException("GetDefaultCommandKit"); }

    // IObject interface
    bool SetName(const std::string&) override                                           { META_FUNCTION_NOT_IMPLEMENTED_RETURN(false); }
    [[nodiscard]] const std::string& GetName() const noexcept override                  { static std::string name; return name; }
    [[nodiscard]] Ptr<IObject>       GetPtr() override                                  { return shared_from_this(); }

private:
    Settings               m_settings;
    FakeDevice             m_fake_device;
    FpsCounter             m_fps_counter;
    FakeObjectRegistry     m_object_registry;
    mutable tf::Executor   m_executor;
};

class FakeRenderPattern
    : public IRenderPattern
    , public Data::Emitter<IObjectCallback>
    , public std::enable_shared_from_this<FakeRenderPattern>
{
public:
    FakeRenderPattern(IRenderContext& render_context) : m_render_context(render_context) { }

    // IRenderPattern interface
    const IRenderContext& GetRenderContext() const noexcept override     { return m_render_context; }
    IRenderContext&       GetRenderContext() noexcept override           { return m_render_context; }
    const Settings&       GetSettings() const noexcept override          { return m_settings; }
    Data::Size            GetAttachmentCount() const noexcept override   { return 0U; }
    AttachmentFormats     GetAttachmentFormats() const noexcept override { return {}; }

    // IObject interface
    bool SetName(const std::string&) override                          { META_FUNCTION_NOT_IMPLEMENTED_RETURN(false); }
    [[nodiscard]] const std::string& GetName() const noexcept override { static std::string name; return name; }
    [[nodiscard]] Ptr<IObject>       GetPtr() override                 { return shared_from_this(); }

private:
    IRenderContext& m_render_context;
    Settings        m_settings;
};

} // namespace Methane::Graphics