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

#include <Methane/Graphics/RHI/IContext.h>
#include <Methane/Graphics/RHI/IObject.h>
#include <Methane/Graphics/RHI/System.h>
#include <Methane/Graphics/RHI/Device.h>
#include <Methane/Data/Receiver.hpp>
#include <Methane/Data/Emitter.hpp>

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <stdexcept>

namespace Methane
{

namespace rhi = Methane::Graphics::Rhi;

static rhi::Device GetTestDevice()
{
    static const rhi::Devices& devices = rhi::System::Get().UpdateGpuDevices();
    if (devices.empty())
        throw std::logic_error("No RHI devices available");

    return devices[0];
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

class ContextCallbackTester final
    : private Data::Receiver<rhi::IContextCallback>
{
public:
    ContextCallbackTester(rhi::IContext& context)
        : m_context(context)
    { dynamic_cast<Data::IEmitter<rhi::IContextCallback>&>(context).Connect(*this); }

    template<typename ContextType>
    ContextCallbackTester(ContextType& context)
        : m_context(context.GetInterface())
    { context.Connect(*this); }

    bool IsContextReleased() const noexcept
    { return m_is_context_released; }

    bool IsContextCompletingInitialization() const noexcept
    { return m_is_context_completing_initialization; }

    bool IsContextInitialized() const noexcept
    { return m_is_context_initialized; }

    void Reset()
    {
        m_is_context_released = false;
        m_is_context_completing_initialization = false;
        m_is_context_initialized = false;
    }

private:
    void OnContextReleased(rhi::IContext& context) override
    {
        CHECK(std::addressof(m_context) == std::addressof(context));
        m_is_context_released = true;
    }

    void OnContextCompletingInitialization(rhi::IContext& context) override
    {
        CHECK(std::addressof(m_context) == std::addressof(context));
        CHECK(context.IsCompletingInitialization());
        m_is_context_completing_initialization = true;
    }

    void OnContextInitialized(rhi::IContext& context) override
    {
        CHECK(std::addressof(m_context) == std::addressof(context));
        m_is_context_initialized = true;
    }

    rhi::IContext& m_context;
    bool m_is_context_released = false;
    bool m_is_context_completing_initialization = false;
    bool m_is_context_initialized = false;
};

} // namespace Methane