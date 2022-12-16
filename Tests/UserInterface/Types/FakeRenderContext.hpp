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
#include <Methane/Graphics/RHI/IDevice.h>
#include <Methane/Graphics/RHI/IRenderPass.h>
#include <Methane/Graphics/RHI/ITransferCommandList.h>
#include <Methane/Graphics/RHI/IRenderCommandList.h>
#include <Methane/Graphics/RHI/IRenderContext.h>
#include <Methane/Graphics/RHI/ICommandListSet.h>
#include <Methane/Graphics/RHI/ICommandQueue.h>
#include <Methane/Graphics/Base/FpsCounter.h>
#include <Methane/Data/Emitter.hpp>

namespace Methane::Graphics
{

class FakeObjectRegistry
    : public Rhi::IObjectRegistry
{
public:
    void                            AddGraphicsObject(Rhi::IObject&) override                     { META_FUNCTION_NOT_IMPLEMENTED(); }
    void                            RemoveGraphicsObject(Rhi::IObject&) override                  { META_FUNCTION_NOT_IMPLEMENTED(); }
    [[nodiscard]] Ptr<Rhi::IObject> GetGraphicsObject(const std::string&) const noexcept override { return nullptr; }
    [[nodiscard]] bool              HasGraphicsObject(const std::string&) const noexcept override { return false; }
};

class FakeDevice
    : public Rhi::IDevice
    , public Data::Emitter<Rhi::IDeviceCallback>
    , public Data::Emitter<Rhi::IObjectCallback>
    , public std::enable_shared_from_this<FakeDevice>
{
public:
    // IDevice interface
    [[nodiscard]] const std::string& GetAdapterName() const noexcept override { static std::string s_name; return s_name; }
    [[nodiscard]] bool IsSoftwareAdapter() const noexcept override { return true; }
    [[nodiscard]] const Capabilities& GetCapabilities() const noexcept override { static const Capabilities s_caps; return s_caps; }
    [[nodiscard]] std::string ToString() const override { return { }; }

    // IObject interface
    bool SetName(std::string_view) override                          { META_FUNCTION_NOT_IMPLEMENTED_RETURN(false); }
    [[nodiscard]] std::string_view GetName() const noexcept override { static std::string name; return name; }
    [[nodiscard]] Ptr<IObject>       GetPtr() override                 { return shared_from_this(); }
};

class FakeCommandQueue
    : public Rhi::ICommandQueue
    , public Data::Emitter<Rhi::IObjectCallback>
    , public std::enable_shared_from_this<FakeCommandQueue>
{
public:
    FakeCommandQueue(const Rhi::IContext& context, Rhi::CommandListType type)
        : m_context(context)
        , m_type(type)
    { }

    // ICommandQueue interface
    [[nodiscard]] const Rhi::IContext&      GetContext() const noexcept override              { return m_context; }
    [[nodiscard]] Rhi::CommandListType      GetCommandListType() const noexcept override      { return m_type; }
    [[nodiscard]] uint32_t                  GetFamilyIndex() const noexcept override          { return 0U; }
    [[nodiscard]] Rhi::ITimestampQueryPool* GetTimestampQueryPool() const noexcept override   { return nullptr; }
    void Execute(Rhi::ICommandListSet&, const Rhi::ICommandList::CompletedCallback&) override { META_FUNCTION_NOT_IMPLEMENTED(); }

    // IObject interface
    bool SetName(std::string_view) override                          { META_FUNCTION_NOT_IMPLEMENTED_RETURN(false); }
    [[nodiscard]] std::string_view GetName() const noexcept override { static std::string name; return name; }
    [[nodiscard]] Ptr<IObject>       GetPtr() override                 { return shared_from_this(); }

private:
    const Rhi::IContext& m_context;
    Rhi::CommandListType m_type;
};

class FakeCommandListSet
    : public Rhi::ICommandListSet
{
public:
    [[nodiscard]] Data::Size                     GetCount() const noexcept override       { return 0; }
    [[nodiscard]] const Refs<Rhi::ICommandList>& GetRefs() const noexcept override        { return m_command_list_refs; }
    [[nodiscard]] Rhi::ICommandList&             operator[](Data::Index) const override   { META_FUNCTION_NOT_IMPLEMENTED(); }

private:
    Refs<Rhi::ICommandList> m_command_list_refs;
};

template<typename CommandListT, Rhi::CommandListType command_list_type>
class FakeCommandList
    : public CommandListT
    , public std::enable_shared_from_this<FakeCommandList<CommandListT, command_list_type>>
{
public:
    FakeCommandList(Rhi::ICommandQueue& command_queue)
        : m_command_queue(command_queue)
    { }

    // ICommandList interface
    [[nodiscard]] Rhi::CommandListType  GetType() const noexcept override     { return command_list_type; }
    [[nodiscard]] Rhi::CommandListState GetState() const noexcept override    { return Rhi::CommandListState::Pending; }
    void PopDebugGroup() override                                             { META_FUNCTION_NOT_IMPLEMENTED(); }
    void PushDebugGroup(Rhi::ICommandListDebugGroup&) override                { META_FUNCTION_NOT_IMPLEMENTED(); }
    void Reset(Rhi::ICommandListDebugGroup*) override                         { META_FUNCTION_NOT_IMPLEMENTED(); }
    void ResetOnce(Rhi::ICommandListDebugGroup*) override                     { META_FUNCTION_NOT_IMPLEMENTED(); }
    void SetProgramBindings(Rhi::IProgramBindings&,
                            Rhi::ProgramBindingsApplyBehaviorMask) override   { META_FUNCTION_NOT_IMPLEMENTED(); }
    void SetResourceBarriers(const Rhi::IResourceBarriers&) override          { META_FUNCTION_NOT_IMPLEMENTED(); }
    void Commit() override                                                    { META_FUNCTION_NOT_IMPLEMENTED(); }
    void WaitUntilCompleted(uint32_t) override                                { META_FUNCTION_NOT_IMPLEMENTED(); }
    [[nodiscard]] Data::TimeRange GetGpuTimeRange(bool) const override        { throw Data::TimeRange{ }; }
    [[nodiscard]] Rhi::ICommandQueue& GetCommandQueue() override              { return m_command_queue; }

    // IObject interface
    bool SetName(std::string_view) override                                   { META_FUNCTION_NOT_IMPLEMENTED_RETURN(false); }
    [[nodiscard]] std::string_view  GetName() const noexcept override         { return {}; }
    [[nodiscard]] Ptr<Rhi::IObject> GetPtr() override                         { return std::enable_shared_from_this<FakeCommandList<CommandListT, command_list_type>>::shared_from_this(); }

private:
    Rhi::ICommandQueue& m_command_queue;
};

using FakeTransferCommandList = FakeCommandList<Rhi::ITransferCommandList, Rhi::CommandListType::Transfer>;

class FakeRenderContext
    : public Rhi::IRenderContext
    , public Data::Emitter<Rhi::IContextCallback>
    , public Data::Emitter<Rhi::IObjectCallback>
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
    [[nodiscard]] const Rhi::IFpsCounter& GetFpsCounter() const noexcept override  { return m_fps_counter; }
    bool SetVSyncEnabled(bool vsync_enabled) override                             { m_settings.vsync_enabled = vsync_enabled; return true; }
    bool SetFrameBuffersCount(uint32_t frame_buffers_count) override              { m_settings.frame_buffers_count = frame_buffers_count; return true; }
    bool SetFullScreen(bool is_full_screen) override                              { m_settings.is_full_screen = is_full_screen; return true; }

    // IContext interface
    [[nodiscard]] Type GetType() const noexcept override                                        { return Type::Render; }
    [[nodiscard]] OptionMask GetOptions() const noexcept override                               { return {}; }
    [[nodiscard]] tf::Executor& GetParallelExecutor() const noexcept override                   { return m_executor; }
    [[nodiscard]] Rhi::IObjectRegistry& GetObjectRegistry() noexcept override                   { return m_object_registry; }
    [[nodiscard]] const Rhi::IObjectRegistry& GetObjectRegistry() const noexcept override       { return m_object_registry; }
    void RequestDeferredAction(DeferredAction) const noexcept override                          { }
    void CompleteInitialization() override                                                      { META_FUNCTION_NOT_IMPLEMENTED(); }
    [[nodiscard]] bool IsCompletingInitialization() const noexcept override                     { return false; }
    void WaitForGpu(WaitFor) override                                                           { META_FUNCTION_NOT_IMPLEMENTED(); }
    void Reset(Rhi::IDevice&) override                                                          { META_FUNCTION_NOT_IMPLEMENTED(); }
    void Reset() override                                                                       { META_FUNCTION_NOT_IMPLEMENTED(); }

    [[nodiscard]] const Rhi::IDevice& GetDevice() const override                                { return m_fake_device; }
    [[nodiscard]] Rhi::ICommandKit& GetDefaultCommandKit(Rhi::CommandListType) const override   { throw Methane::NotImplementedException("GetDefaultCommandKit"); }
    [[nodiscard]] Rhi::ICommandKit& GetDefaultCommandKit(Rhi::ICommandQueue&) const override    { throw Methane::NotImplementedException("GetDefaultCommandKit"); }

    // IObject interface
    bool SetName(std::string_view) override                          { META_FUNCTION_NOT_IMPLEMENTED_RETURN(false); }
    [[nodiscard]] std::string_view GetName() const noexcept override { static std::string name; return name; }
    [[nodiscard]] Ptr<IObject>     GetPtr() override                 { return shared_from_this(); }

private:
    Settings             m_settings;
    FakeDevice           m_fake_device;
    Base::FpsCounter     m_fps_counter;
    FakeObjectRegistry   m_object_registry;
    mutable tf::Executor m_executor;
};

class FakeRenderPattern
    : public Rhi::IRenderPattern
    , public Data::Emitter<Rhi::IObjectCallback>
    , public std::enable_shared_from_this<FakeRenderPattern>
{
public:
    FakeRenderPattern(Rhi::IRenderContext& render_context) : m_render_context(render_context) { }

    // IRenderPattern interface
    const Rhi::IRenderContext& GetRenderContext() const noexcept override     { return m_render_context; }
    Rhi::IRenderContext&       GetRenderContext() noexcept override           { return m_render_context; }
    const Settings&            GetSettings() const noexcept override          { return m_settings; }
    Data::Size                 GetAttachmentCount() const noexcept override   { return 0U; }
    AttachmentFormats          GetAttachmentFormats() const noexcept override { return {}; }

    // IObject interface
    bool SetName(std::string_view) override                          { META_FUNCTION_NOT_IMPLEMENTED_RETURN(false); }
    [[nodiscard]] std::string_view GetName() const noexcept override { static std::string name; return name; }
    [[nodiscard]] Ptr<Rhi::IObject>  GetPtr() override                 { return shared_from_this(); }

private:
    Rhi::IRenderContext& m_render_context;
    Settings             m_settings;
};

} // namespace Methane::Graphics