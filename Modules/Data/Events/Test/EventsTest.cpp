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

FILE: Test/EventsTest.cpp
Unit tests of event connections with Emitter and Receiver classes

******************************************************************************/

#include <catch2/catch.hpp>

#include <Methane/Data/Emitter.hpp>
#include <Methane/Memory.hpp>

#include <array>

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
    void Bind(TestEmitter& emitter)
    {
        emitter.Connect(*this);
    }

    void Unbind(TestEmitter& emitter)
    {
        emitter.Disconnect(*this);
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

} // namespace Methane::Data

using namespace Methane::Data;

TEST_CASE("Connect 1 Emitter to 1 Receiver", "[events]")
{
    SECTION("Emit without arguments")
    {
        TestEmitter  emitter;
        TestReceiver receiver;

        CHECK_NOTHROW(receiver.Bind(emitter));

        CHECK(!receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());

        CHECK_NOTHROW(emitter.EmitFoo());

        CHECK(receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());
    }

    SECTION("Emit with arguments")
    {
        TestEmitter  emitter;
        TestReceiver receiver;

        CHECK_NOTHROW(receiver.Bind(emitter));

        CHECK(!receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());
        CHECK(receiver.GetBarA() == 0);
        CHECK(receiver.GetBarB() == false);
        CHECK(receiver.GetBarC() == 0.f);

        CHECK_NOTHROW(emitter.EmitBar(1, true, 2.3f));

        CHECK(!receiver.IsFooCalled());
        CHECK(receiver.IsBarCalled());
        CHECK(receiver.GetBarA() == 1);
        CHECK(receiver.GetBarB() == true);
        CHECK(receiver.GetBarC() == 2.3f);
    }

    SECTION("Emit after disconnect")
    {
        TestEmitter  emitter;
        TestReceiver receiver;

        CHECK_NOTHROW(receiver.Bind(emitter));

        CHECK(!receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());

        CHECK_NOTHROW(receiver.Unbind(emitter));
        CHECK_NOTHROW(emitter.EmitFoo());

        CHECK(!receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());
    }

    SECTION("Emit after receiver destroyed")
    {
        TestEmitter  emitter;
        {
            TestReceiver receiver;
            CHECK_NOTHROW(receiver.Bind(emitter));
        }
        CHECK_NOTHROW(emitter.EmitFoo());
    }

    SECTION("Bound emitter destroyed")
    {
        TestReceiver receiver;
        {
            TestEmitter emitter;
            CHECK_NOTHROW(receiver.Bind(emitter));
        }
    }
}

TEST_CASE("Connect 1 Emitter to Many Receivers", "[events]")
{
    SECTION("Emit without arguments")
    {
        TestEmitter emitter;
        std::array<TestReceiver, 10> receivers;

        for(TestReceiver& receiver : receivers)
        {
            CHECK_NOTHROW(receiver.Bind(emitter));
            CHECK(!receiver.IsFooCalled());
            CHECK(!receiver.IsBarCalled());
        }

        CHECK_NOTHROW(emitter.EmitFoo());

        for(TestReceiver& receiver : receivers)
        {
            CHECK(receiver.IsFooCalled());
            CHECK(!receiver.IsBarCalled());
        }
    }

    SECTION("Emit with arguments")
    {
        TestEmitter emitter;
        std::array<TestReceiver, 5> receivers;

        for(TestReceiver& receiver : receivers)
        {
            CHECK_NOTHROW(receiver.Bind(emitter));
            CHECK(!receiver.IsFooCalled());
            CHECK(!receiver.IsBarCalled());
            CHECK(receiver.GetBarA() == 0);
            CHECK(receiver.GetBarB() == false);
            CHECK(receiver.GetBarC() == 0.f);
        }

        CHECK_NOTHROW(emitter.EmitBar(1, true, 2.3f));

        for(TestReceiver& receiver : receivers)
        {
            CHECK(!receiver.IsFooCalled());
            CHECK(receiver.IsBarCalled());
            CHECK(receiver.GetBarA() == 1);
            CHECK(receiver.GetBarB() == true);
            CHECK(receiver.GetBarC() == 2.3f);
        }
    }
}

TEST_CASE("Connect Many Emitters to 1 Receiver", "[events]")
{
    SECTION("Emit without arguments")
    {
        std::array<TestEmitter, 5> emitters;
        TestReceiver receiver;

        for(TestEmitter& emitter : emitters)
        {
            CHECK_NOTHROW(receiver.Bind(emitter));
        }

        CHECK(!receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());

        uint32_t emit_count = 0u;
        for(TestEmitter& emitter : emitters)
        {
            CHECK_NOTHROW(emitter.EmitFoo());

            emit_count++;
            CHECK(receiver.GetFooCallCount() == emit_count);
        }

        CHECK(!receiver.IsBarCalled());
    }

    SECTION("Emit with arguments")
    {
        std::array<TestEmitter, 5> emitters;
        TestReceiver receiver;

        for(TestEmitter& emitter : emitters)
        {
            CHECK_NOTHROW(receiver.Bind(emitter));
        }

        CHECK(!receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());
        CHECK(receiver.GetBarA() == 0);
        CHECK(receiver.GetBarB() == false);
        CHECK(receiver.GetBarC() == 0.f);

        int a = 1;
        bool b = true;
        float    c          = 2.3f;
        uint32_t emit_count = 0;

        for(TestEmitter& emitter : emitters)
        {
            CHECK_NOTHROW(emitter.EmitBar(a, b, c));

            emit_count++;
            CHECK(receiver.GetBarCallCount() == emit_count);
            CHECK(receiver.GetBarA() == a);
            CHECK(receiver.GetBarB() == b);
            CHECK(receiver.GetBarC() == c);

            a++;
            b = !b;
            c = c * 2.f;
        }

        CHECK(!receiver.IsFooCalled());
    }
}