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

namespace Methane::Data
{

struct ITestEvents
{
    virtual void Foo() = 0;
    virtual void Bar(int a, bool b, float c) = 0;

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
};

class TestReceiver : protected Receiver<ITestEvents>
{
public:
    void Bind(TestEmitter& emitter, bool new_connection = true)
    {
        const size_t connected_receivers_count = emitter.GetConnectedReceiversCount();

        emitter.Connect(*this);

        CHECK(emitter.GetConnectedReceiversCount() == connected_receivers_count + static_cast<size_t>(new_connection));
    }

    void Unbind(TestEmitter& emitter, bool existing_connection = true)
    {
        const size_t connected_receivers_count = emitter.GetConnectedReceiversCount();

        emitter.Disconnect(*this);

        CHECK(emitter.GetConnectedReceiversCount() == connected_receivers_count - static_cast<size_t>(existing_connection));
    }

    bool     IsFooCalled() const     { return m_foo_call_count > 0u; }
    uint32_t GetFooCallCount() const { return m_foo_call_count; }

    bool     IsBarCalled() const     { return m_bar_call_count > 0u; }
    uint32_t GetBarCallCount() const { return m_bar_call_count; }

    int      GetBarA() const         { return m_bar_a; }
    bool     GetBarB() const         { return m_bar_b; }
    float    GetBarC() const         { return m_bar_c; }

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

private:
    uint32_t  m_foo_call_count = 0u;
    uint32_t  m_bar_call_count = 0u;
    int       m_bar_a = 0;
    bool      m_bar_b = false;
    float     m_bar_c = 0.f;
};

constexpr int   g_bar_a = 1;
constexpr bool  g_bar_b = true;
constexpr float g_bar_c = 2.3f;

} // namespace Methane::Data