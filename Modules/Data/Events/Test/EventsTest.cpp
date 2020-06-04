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

#include "EventWrappers.hpp"

#include <catch2/catch.hpp>

#include <array>

using namespace Methane;
using namespace Methane::Data;

TEST_CASE("Connect one emitter to one receiver", "[events]")
{
    SECTION("Emit without arguments")
    {
        TestEmitter  emitter;
        TestReceiver receiver;

        receiver.CheckBind(emitter);

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

        receiver.CheckBind(emitter);

        CHECK(!receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());
        CHECK(receiver.GetBarA() == 0);
        CHECK(receiver.GetBarB() == false);
        CHECK(receiver.GetBarC() == 0.f);

        CHECK_NOTHROW(emitter.EmitBar(g_bar_a, g_bar_b, g_bar_c));

        CHECK(!receiver.IsFooCalled());
        CHECK(receiver.IsBarCalled());
        CHECK(receiver.GetBarA() == g_bar_a);
        CHECK(receiver.GetBarB() == g_bar_b);
        CHECK(receiver.GetBarC() == g_bar_c);
    }

    SECTION("Emit after disconnect")
    {
        TestEmitter  emitter;
        TestReceiver receiver;

        receiver.CheckBind(emitter);

        CHECK(!receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());

        receiver.CheckUnbind(emitter);
        CHECK_NOTHROW(emitter.EmitFoo());

        CHECK(!receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());
    }

    SECTION("Emit after receiver destroyed")
    {
        TestEmitter  emitter;
        {
            TestReceiver receiver;
            receiver.CheckBind(emitter);
        }
        CHECK_NOTHROW(emitter.EmitFoo());
    }

    SECTION("Bound emitter destroyed")
    {
        TestReceiver receiver;
        {
            TestEmitter emitter;
            receiver.CheckBind(emitter);
        }
    }
}

TEST_CASE("Connect one emitter to many receivers", "[events]")
{
    SECTION("Emit without arguments")
    {
        TestEmitter emitter;
        std::array<TestReceiver, 5> receivers;

        for(TestReceiver& receiver : receivers)
        {
            receiver.CheckBind(emitter);
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
            receiver.CheckBind(emitter);
            CHECK(!receiver.IsFooCalled());
            CHECK(!receiver.IsBarCalled());
            CHECK(receiver.GetBarA() == 0);
            CHECK(receiver.GetBarB() == false);
            CHECK(receiver.GetBarC() == 0.f);
        }

        CHECK_NOTHROW(emitter.EmitBar(g_bar_a, g_bar_b, g_bar_c));

        for(TestReceiver& receiver : receivers)
        {
            CHECK(!receiver.IsFooCalled());
            CHECK(receiver.IsBarCalled());
            CHECK(receiver.GetBarA() == g_bar_a);
            CHECK(receiver.GetBarB() == g_bar_b);
            CHECK(receiver.GetBarC() == g_bar_c);
        }
    }

    SECTION("Copied receivers are connected to emitter")
    {
        TestEmitter emitter;
        TestReceiver receiver;
        receiver.CheckBind(emitter);

        std::vector<TestReceiver> receiver_copies;
        for(size_t id = 0; id < 5; ++id)
        {
            CHECK_NOTHROW(receiver_copies.push_back(receiver));
        }

        CHECK(emitter.GetConnectedReceiversCount() == receiver_copies.size() + 1);
        CHECK_NOTHROW(emitter.EmitFoo());

        CHECK(receiver.IsFooCalled());
        for(TestReceiver& receiver_copy : receiver_copies)
        {
            CHECK(receiver_copy.IsFooCalled());
        }
    }

    SECTION("Connect receivers during emitted call")
    {
        TestEmitter emitter;
        std::array<TestReceiver, 5> receivers;

        for(TestReceiver& receiver : receivers)
        {
            receiver.CheckBind(emitter);
        }

        CHECK(emitter.GetConnectedReceiversCount() == receivers.size());
        Ptrs<TestReceiver> dynamic_receivers;

        CHECK_NOTHROW(emitter.EmitCall([&dynamic_receivers, &emitter](size_t)
        {
            auto new_receiver_ptr = std::make_shared<TestReceiver>();
            new_receiver_ptr->CheckBind(emitter);
            dynamic_receivers.emplace_back(std::move(new_receiver_ptr));
        }));

        const size_t total_receivers_count = dynamic_receivers.size() + receivers.size();
        CHECK(dynamic_receivers.size() == receivers.size());
        CHECK(emitter.GetConnectedReceiversCount() == total_receivers_count);
    }

    SECTION("Destroy receivers during emitted call")
    {
        TestEmitter emitter;
        Ptrs<TestReceiver> receivers_ptrs(5);

        size_t receiver_index = 0;
        for(Ptr<TestReceiver>& receiver_ptr : receivers_ptrs)
        {
            receiver_ptr = std::make_shared<TestReceiver>(receiver_index++);
            receiver_ptr->CheckBind(emitter);
        }

        CHECK_NOTHROW(emitter.EmitCall([&receivers_ptrs](size_t receiver_index)
        {
             receivers_ptrs[receiver_index].reset();
        }));

        CHECK(emitter.GetConnectedReceiversCount() == 0);
    }
}

TEST_CASE("Connect many emitters to one receiver", "[events]")
{
    SECTION("Emit without arguments")
    {
        std::array<TestEmitter, 5> emitters;
        TestReceiver receiver;

        for(TestEmitter& emitter : emitters)
        {
            receiver.CheckBind(emitter);
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
            receiver.CheckBind(emitter);
        }

        CHECK(!receiver.IsFooCalled());
        CHECK(!receiver.IsBarCalled());
        CHECK(receiver.GetBarA() == 0);
        CHECK(receiver.GetBarB() == false);
        CHECK(receiver.GetBarC() == 0.f);

        uint32_t emit_count = 0;
        int      bar_a = g_bar_a;
        bool     bar_b = g_bar_b;
        float    bar_c = g_bar_c;

        for(TestEmitter& emitter : emitters)
        {
            CHECK_NOTHROW(emitter.EmitBar(bar_a, bar_b, bar_c));

            emit_count++;
            CHECK(receiver.GetBarCallCount() == emit_count);
            CHECK(receiver.GetBarA() == bar_a);
            CHECK(receiver.GetBarB() == bar_b);
            CHECK(receiver.GetBarC() == bar_c);

            bar_a++;
            bar_b = !bar_b;
            bar_c *= 2.f;
        }

        CHECK(!receiver.IsFooCalled());
    }
}