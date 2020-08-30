/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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

#include <catch2/catch.hpp>

#include <Methane/Data/Emitter.hpp>

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

class TestEmitter : public Emitter<ITestEvents>
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

class TestReceiver : protected Receiver<ITestEvents>
{
public:
    TestReceiver() = default;
    explicit TestReceiver(size_t id) : m_id(id) { }

    void Bind(TestEmitter& emitter)
    {
        emitter.Connect(*this);
    }

    void Unbind(TestEmitter& emitter)
    {
        emitter.Disconnect(*this);
    }

    void CheckBind(TestEmitter& emitter, bool new_connection = true)
    {
        const size_t connected_receivers_count = emitter.GetConnectedReceiversCount();
        const size_t connected_emitters_count  = GetConnectedEmittersCount();

        CHECK_NOTHROW(Bind(emitter));

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

    size_t   GetId() const           { return m_id; }

    bool     IsFooCalled() const     { return m_foo_call_count > 0u; }
    uint32_t GetFooCallCount() const { return m_foo_call_count; }

    bool     IsBarCalled() const     { return m_bar_call_count > 0u; }
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
    }

    void Bar(int a, bool b, float c) override
    {
        m_bar_call_count++;
        m_bar_a = a;
        m_bar_b = b;
        m_bar_c = c;
    }

    void Call(const CallFunc& f) override
    {
        m_func_call_count++;
        f(m_id);
    }

private:
    const size_t m_id = 0;
    uint32_t     m_foo_call_count = 0u;
    uint32_t     m_bar_call_count = 0u;
    uint32_t     m_func_call_count = 0u;
    int          m_bar_a = 0;
    bool         m_bar_b = false;
    float        m_bar_c = 0.f;
};

constexpr int   g_bar_a = 1;
constexpr bool  g_bar_b = true;
constexpr float g_bar_c = 2.3f;

} // namespace Methane::Data