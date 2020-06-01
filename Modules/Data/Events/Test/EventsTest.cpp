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
    void BindTo(TestEmitter& emitter)
    {
        emitter.Connect(*this);
    }

    bool  IsFooCalled() const { return m_foo_called; }
    bool  IsBarCalled() const { return m_bar_called; }
    int   GetBarA() const     { return m_bar_a; }
    bool  GetBarB() const     { return m_bar_b; }
    float GetBarC() const     { return m_bar_c; }

protected:
    // ITestEvent implementation
    void Foo() override
    {
        m_foo_called = true;
    }

    void Bar(int a, bool b, float c) override
    {
        m_bar_called = true;
        m_bar_a = a;
        m_bar_b = b;
        m_bar_c = c;
    }

private:
    bool  m_foo_called = false;
    bool  m_bar_called = false;
    int   m_bar_a = 0;
    bool  m_bar_b = false;
    float m_bar_c = 0.f;
};

} // namespace Methane::Data

using namespace Methane::Data;

TEST_CASE("Emitter and Receiver connection", "[events]")
{
    SECTION("Foo no args call with 1-1 connection")
    {
        TestEmitter  emitter;
        TestReceiver receiver;
        receiver.BindTo(emitter);

        CHECK(!receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());

        emitter.EmitFoo();

        CHECK(receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());
    }

    SECTION("Bar args call with 1-1 connection")
    {
        TestEmitter  emitter;
        TestReceiver receiver;
        receiver.BindTo(emitter);

        CHECK(!receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());
        CHECK(receiver.GetBarA() == 0);
        CHECK(receiver.GetBarB() == false);
        CHECK(receiver.GetBarC() == 0.f);

        emitter.EmitBar(1, true, 2.3f);

        CHECK(!receiver.IsFooCalled());
        CHECK(receiver.IsBarCalled());
        CHECK(receiver.GetBarA() == 1);
        CHECK(receiver.GetBarB() == true);
        CHECK(receiver.GetBarC() == 2.3f);
    }
}
