/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Tests/Graphics/RHI/RhiTestHelpers.hpp
RHI Test helper classes

******************************************************************************/
#include <Methane/Graphics/RHI/IObject.h>
#include <Methane/Graphics/RHI/IDevice.h>
#include <Methane/Graphics/RHI/IContext.h>
#include <Methane/Graphics/RHI/ICommandList.h>
#include <Methane/Graphics/RHI/IResource.h>
#include <Methane/Graphics/RHI/IRenderPass.h>
#include <Methane/Graphics/RHI/System.h>
#include <Methane/Graphics/RHI/Device.h>
#include <Methane/Graphics/Base/Object.h>
#include <Methane/Data/Receiver.hpp>
#include <Methane/Data/Emitter.hpp>

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <stdexcept>

namespace Methane
{

namespace rhi = Methane::Graphics::Rhi;

[[maybe_unused]]
static rhi::Device GetTestDevice()
{
    static const rhi::Devices& devices = rhi::System::Get().UpdateGpuDevices();
    if (devices.empty())
        throw std::logic_error("No RHI devices available");

    return devices[0];
}

template<typename BaseCommandListType, typename BufferSetPimplType, typename CommandListPimplType>
bool IsResourceRetainedByCommandList(BufferSetPimplType& buffer_set, const CommandListPimplType& cmd_list)
{
    const auto& null_cmd_list = dynamic_cast<const BaseCommandListType&>(cmd_list.GetInterface());
    const Ptrs<Graphics::Base::Object>& retained_resources = null_cmd_list.GetCommandState().retained_resources;
    return std::ranges::find(retained_resources, dynamic_cast<Graphics::Base::Object&>(buffer_set.GetInterface()).GetBasePtr())
        != retained_resources.end();
}

class ObjectCallbackTester final
    : private Data::Receiver<rhi::IObjectCallback>
{
public:
    ObjectCallbackTester(rhi::IObject& obj)
        : m_obj(obj)
    { obj.Connect(*this); }

    template<typename ObjectType>
    ObjectCallbackTester(ObjectType& obj)
        : m_obj(obj.GetInterface())
    { obj.Connect(*this); }

    bool IsObjectDestroyed() const noexcept
    { return m_is_object_destroyed; }

    bool IsObjectNameChanged() const noexcept
    { return m_is_object_name_changed; }

    const std::string& GetOldObjectName() const noexcept
    { return m_old_name; }

    const std::string& GetCurObjectName() const noexcept
    { return m_cur_name; }

    void ResetObjectNameChanged()
    { m_is_object_name_changed = false; }

private:
    void OnObjectNameChanged(rhi::IObject& obj, const std::string& old_name) override
    {
        CHECK(std::addressof(obj) == std::addressof(m_obj));
        m_is_object_name_changed = true;
        m_old_name = old_name;
        m_cur_name = obj.GetName();
    }

    void OnObjectDestroyed(rhi::IObject& obj) override
    {
        CHECK(std::addressof(obj) == std::addressof(m_obj));
        m_is_object_destroyed = true;
    }

    rhi::IObject& m_obj;
    bool m_is_object_destroyed = false;
    bool m_is_object_name_changed = false;
    std::string m_old_name;
    std::string m_cur_name;
};

class DeviceCallbackTester final
    : private Data::Receiver<rhi::IDeviceCallback>
{
public:
    DeviceCallbackTester(rhi::IDevice& device)
        : m_device(device)
    {
        dynamic_cast<Data::IEmitter<rhi::IDeviceCallback>&>(device).Connect(*this);
    }

    bool IsDeviceRemovalRequested() const noexcept
    {
        return m_is_device_removal_requested;
    }

    bool IsDeviceRemoved() const noexcept
    {
        return m_is_device_removed;
    }

    void Reset()
    {
        m_is_device_removal_requested = false;
        m_is_device_removed = false;
    }

private:
    void OnDeviceRemovalRequested(rhi::IDevice& device) override
    {
        CHECK(std::addressof(m_device) == std::addressof(device));
        m_is_device_removal_requested = true;
    }

    void OnDeviceRemoved(rhi::IDevice& device) override
    {
        CHECK(std::addressof(m_device) == std::addressof(device));
        m_is_device_removed = true;
    }

    rhi::IDevice& m_device;
    bool m_is_device_removal_requested = false;
    bool m_is_device_removed = false;
};

class ContextCallbackTester final
    : private Data::Receiver<rhi::IContextCallback>
{
public:
    ContextCallbackTester(rhi::IContext& context)
        : m_context(context)
    {
        dynamic_cast<Data::IEmitter<rhi::IContextCallback>&>(context).Connect(*this);
    }

    template<typename ContextType>
    ContextCallbackTester(ContextType& context)
        : m_context(context.GetInterface())
    {
        context.Connect(*this);
    }

    bool IsContextReleased() const noexcept
    {
        return m_is_context_released;
    }

    bool IsContextUploadingResources() const noexcept
    {
        return m_is_context_uploading_resources;
    }

    bool IsContextInitialized() const noexcept
    {
        return m_is_context_initialized;
    }

    void Reset()
    {
        m_is_context_released = false;
        m_is_context_uploading_resources = false;
        m_is_context_initialized = false;
    }

private:
    void OnContextReleased(rhi::IContext& context) override
    {
        CHECK(std::addressof(m_context) == std::addressof(context));
        m_is_context_released = true;
    }

    void OnContextUploadingResources(rhi::IContext& context) override
    {
        CHECK(std::addressof(m_context) == std::addressof(context));
        m_is_context_uploading_resources = true;
    }

    void OnContextInitialized(rhi::IContext& context) override
    {
        CHECK(std::addressof(m_context) == std::addressof(context));
        m_is_context_initialized = true;
    }

    rhi::IContext& m_context;
    bool m_is_context_released = false;
    bool m_is_context_uploading_resources = false;
    bool m_is_context_initialized = false;
};

class CommandListCallbackTester final
    : private Data::Receiver<rhi::ICommandListCallback>
{
public:
    CommandListCallbackTester(rhi::ICommandList& cmd_list)
        : m_cmd_list(cmd_list)
    { dynamic_cast<Data::IEmitter<rhi::ICommandListCallback>&>(cmd_list).Connect(*this); }

    template<typename CommandListType>
    CommandListCallbackTester(CommandListType& cmd_list)
        : m_cmd_list(cmd_list.GetInterface())
    { cmd_list.Connect(*this); }

    bool IsStateChanged() const noexcept { return m_is_state_changed; }
    bool IsExecutionCompleted() const noexcept { return m_is_execution_completed; }
    rhi::CommandListState GetTrackingState() const noexcept { return m_state; }

    void Reset()
    {
        m_is_state_changed = false;
        m_is_execution_completed = false;
        m_state = rhi::CommandListState::Pending;
    }

private:
    void OnCommandListStateChanged(rhi::ICommandList& cmd_list) override
    {
        CHECK(std::addressof(cmd_list) == std::addressof(m_cmd_list));
        m_is_state_changed = true;
        m_state = cmd_list.GetState();
    }

    void OnCommandListExecutionCompleted(rhi::ICommandList& cmd_list) override
    {
        CHECK(std::addressof(cmd_list) == std::addressof(m_cmd_list));
        CHECK(cmd_list.GetState() == rhi::CommandListState::Pending);
        m_is_execution_completed = true;
        m_state = cmd_list.GetState();
    }

    rhi::ICommandList& m_cmd_list;
    bool m_is_state_changed = false;
    bool m_is_execution_completed = false;
    rhi::CommandListState m_state = rhi::CommandListState::Pending;
};

class ResourceCallbackTester final
    : private Data::Receiver<rhi::IResourceCallback>
{
public:
    ResourceCallbackTester(rhi::IResource& resource)
        : m_resource(resource)
    { dynamic_cast<Data::IEmitter<rhi::IResourceCallback>&>(resource).Connect(*this); }

    template<typename ResourceType>
    ResourceCallbackTester(ResourceType& resource)
        : m_resource(resource.GetInterface())
    { resource.Connect(*this); }

    bool IsResourceReleased() const noexcept { return m_is_resource_released; }

    void Reset()
    {
        m_is_resource_released = false;
    }

private:
    void OnResourceReleased(rhi::IResource& resource) override
    {
        CHECK(std::addressof(resource) == std::addressof(m_resource));
        m_is_resource_released = true;
    }

    rhi::IResource& m_resource;
    bool m_is_resource_released = false;
};

class RenderPassCallbackTester final
    : private Data::Receiver<rhi::IRenderPassCallback>
{
public:
    RenderPassCallbackTester(rhi::IRenderPass& render_pass)
        : m_render_pass(render_pass)
    { dynamic_cast<Data::IEmitter<rhi::IRenderPassCallback>&>(render_pass).Connect(*this); }

    template<typename RenderPassType>
    RenderPassCallbackTester(RenderPassType& render_pass)
        : m_render_pass(render_pass.GetInterface())
    { render_pass.Connect(*this); }

    bool IsRenderPassUpdated() const noexcept { return m_is_render_pass_updated; }

    void Reset() { m_is_render_pass_updated = false; }

private:
    void OnRenderPassUpdated(const rhi::IRenderPass& render_pass) override
    {
        CHECK(std::addressof(render_pass) == std::addressof(m_render_pass));
        m_is_render_pass_updated = true;
    }

    rhi::IRenderPass& m_render_pass;
    bool m_is_render_pass_updated = false;
};

} // namespace Methane