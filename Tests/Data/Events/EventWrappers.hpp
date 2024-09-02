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

FILE: Test/EventWrappers.hpp
Emitter and Recever wrappers for events testing.

******************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <Methane/Data/Emitter.hpp>
#include <Methane/Data/Transmitter.hpp>

#include <functional>

namespace Methane::Data
{

struct ITestEvents
{
    using CallFunc = std::function<void(size_t /*receiver_id*/)>;

    virtual void Foo() = 0;
    virtual void Bar(int a, bool b, float c) = 0;
    virtual void Call(const CallFunc& f) = 0;

    virtual ~ITestEvents() = default;
};

class TestEmitter
    : public Emitter<ITestEvents>
{
public:
    void EmitFoo()
    {
        Emit(&ITestEvents::Foo);
    }

    void EmitBar(int a, bool b, float c)
    {
        Emit(&ITestEvents::Bar, a, b, c);
    }

    void EmitCall(const ITestEvents::CallFunc& f)
    {
        Emit(&ITestEvents::Call, f);
    }

    using Emitter<ITestEvents>::GetConnectedReceiversCount;
};

class TestTransmitter
    : public Transmitter<ITestEvents>
{
public:
    using Transmitter::Transmitter;
    using Transmitter::Reset;
};

class TestReceiver
    : public Receiver<ITestEvents>
{
public:
    using Ids = std::vector<uint32_t>;

    static const Ids& GetCalledReceiverIds() { return s_called_receiver_ids; }
    static void ClearCalledReceiverIds() { s_called_receiver_ids.clear(); }

    TestReceiver() = default;
    explicit TestReceiver(uint32_t id, bool register_called_ids = false)
        : m_id(id)
        , m_register_called_ids(register_called_ids)
    { }

    void Bind(TestEmitter& emitter, uint32_t priority = 0U)
    {
        emitter.Connect(*this, priority);
    }

    void Unbind(TestEmitter& emitter)
    {
        emitter.Disconnect(*this);
    }

    void CheckBind(TestEmitter& emitter, uint32_t priority = 0U, bool new_connection = true)
    {
        const size_t connected_receivers_count = emitter.GetConnectedReceiversCount();
        const size_t connected_emitters_count  = GetConnectedEmittersCount();

        CHECK_NOTHROW(Bind(emitter, priority));

        CHECK(emitter.GetConnectedReceiversCount() == connected_receivers_count + static_cast<size_t>(new_connection));
        CHECK(GetConnectedEmittersCount()          == connected_emitters_count  + static_cast<size_t>(new_connection));
    }

    void CheckUnbind(TestEmitter& emitter, bool existing_connection = true)
    {
        const size_t connected_receivers_count = emitter.GetConnectedReceiversCount();
        const size_t connected_emitters_count  = GetConnectedEmittersCount();

        CHECK_NOTHROW(Unbind(emitter));

        CHECK(emitter.GetConnectedReceiversCount() == connected_receivers_count - static_cast<size_t>(existing_connection));
        CHECK(GetConnectedEmittersCount()          == connected_emitters_count  - static_cast<size_t>(existing_connection));
    }

    uint32_t GetId() const           { return m_id; }

    bool     IsFooCalled() const     { return m_foo_call_count > 0U; }
    uint32_t GetFooCallCount() const { return m_foo_call_count; }

    bool     IsBarCalled() const     { return m_bar_call_count > 0U; }
    uint32_t GetBarCallCount() const { return m_bar_call_count; }

    uint32_t GetFuncCallCount() const { return m_func_call_count; }

    int      GetBarA() const         { return m_bar_a; }
    bool     GetBarB() const         { return m_bar_b; }
    float    GetBarC() const         { return m_bar_c; }

    using Receiver<ITestEvents>::GetConnectedEmittersCount;

protected:
    // ITestEvent implementation
    void Foo() override
    {
        m_foo_call_count++;
        if (m_register_called_ids)
            s_called_receiver_ids.emplace_back(m_id);
    }

    void Bar(int a, bool b, float c) override
    {
        m_bar_call_count++;
        m_bar_a = a;
        m_bar_b = b;
        m_bar_c = c;
        if (m_register_called_ids)
            s_called_receiver_ids.emplace_back(m_id);
    }

    void Call(const CallFunc& f) override
    {
        m_func_call_count++;
        f(m_id);
    }

private:
    static inline Ids s_called_receiver_ids;

    const uint32_t m_id = 0;
    const bool     m_register_called_ids = false;
    uint32_t       m_foo_call_count = 0U;
    uint32_t       m_bar_call_count = 0U;
    uint32_t       m_func_call_count = 0U;
    int            m_bar_a = 0;
    bool           m_bar_b = false;
    float          m_bar_c = 0.f;
};

constexpr int   g_bar_a = 1;
constexpr bool  g_bar_b = true;
constexpr float g_bar_c = 2.3F;

} // namespace Methane::Data