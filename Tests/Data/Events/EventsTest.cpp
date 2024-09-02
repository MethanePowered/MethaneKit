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

FILE: Test/EventsTest.cpp
Unit tests of event connections with Emitter and Receiver classes

******************************************************************************/

#include "EventWrappers.hpp"

#include <Methane/Data/Transmitter.hpp>

#include <catch2/catch_test_macros.hpp>

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

        CHECK_FALSE(receiver.IsFooCalled());
        CHECK_FALSE(receiver.IsBarCalled());

        CHECK_NOTHROW(emitter.EmitFoo());

        CHECK(receiver.IsFooCalled());
        CHECK_FALSE(receiver.IsBarCalled());
    }

    SECTION("Emit with arguments")
    {
        TestEmitter  emitter;
        TestReceiver receiver;

        receiver.CheckBind(emitter);

        CHECK_FALSE(receiver.IsFooCalled());
        CHECK_FALSE(receiver.IsBarCalled());
        CHECK(receiver.GetBarA() == 0);
        CHECK(receiver.GetBarB() == false);
        CHECK(receiver.GetBarC() == 0.f);

        CHECK_NOTHROW(emitter.EmitBar(g_bar_a, g_bar_b, g_bar_c));

        CHECK_FALSE(receiver.IsFooCalled());
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

        CHECK_FALSE(receiver.IsFooCalled());
        CHECK_FALSE(receiver.IsBarCalled());

        receiver.CheckUnbind(emitter);
        CHECK_NOTHROW(emitter.EmitFoo());

        CHECK_FALSE(receiver.IsFooCalled());
        CHECK_FALSE(receiver.IsBarCalled());
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
            CHECK_FALSE(receiver.IsFooCalled());
            CHECK_FALSE(receiver.IsBarCalled());
        }

        CHECK_NOTHROW(emitter.EmitFoo());

        for(TestReceiver& receiver : receivers)
        {
            CHECK(receiver.IsFooCalled());
            CHECK_FALSE(receiver.IsBarCalled());
        }
    }

    SECTION("Emit by priority")
    {
        TestEmitter emitter;
        std::array<TestReceiver, 5> receivers{
            TestReceiver(1, true),
            TestReceiver(3, true),
            TestReceiver(5, true),
            TestReceiver(2, true),
            TestReceiver(4, true),
        };

        for(TestReceiver& receiver : receivers)
        {
            receiver.Bind(emitter, receiver.GetId());
        }

        TestReceiver::ClearCalledReceiverIds();
        CHECK_NOTHROW(emitter.EmitFoo());

        const TestReceiver::Ids expected_calls_order{ 5, 4, 3, 2, 1 };
        CHECK(TestReceiver::GetCalledReceiverIds() == expected_calls_order);
    }

    SECTION("Emit with arguments")
    {
        TestEmitter emitter;
        std::array<TestReceiver, 5> receivers;

        for(TestReceiver& receiver : receivers)
        {
            receiver.CheckBind(emitter);
            CHECK_FALSE(receiver.IsFooCalled());
            CHECK_FALSE(receiver.IsBarCalled());
            CHECK(receiver.GetBarA() == 0);
            CHECK(receiver.GetBarB() == false);
            CHECK(receiver.GetBarC() == 0.f);
        }

        CHECK_NOTHROW(emitter.EmitBar(g_bar_a, g_bar_b, g_bar_c));

        for(TestReceiver& receiver : receivers)
        {
            CHECK_FALSE(receiver.IsFooCalled());
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

    SECTION("Emit receivers connected during emitted call")
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

            // Emit Foo call during the other emit right after new receiver connection
            emitter.EmitFoo(); // this should call new_receiver_ptr->Foo()
        }));

        // Check that all dynamic receivers received Foo calls
        for(size_t dynamic_receiver_index = 0; dynamic_receiver_index < dynamic_receivers.size(); ++dynamic_receiver_index)
        {
            CHECK(dynamic_receivers[dynamic_receiver_index]->GetFooCallCount() == receivers.size() - dynamic_receiver_index);
        }

        const size_t total_receivers_count = dynamic_receivers.size() + receivers.size();
        CHECK(dynamic_receivers.size() == receivers.size());
        CHECK(emitter.GetConnectedReceiversCount() == total_receivers_count);
    }

    SECTION("Destroy receivers during emitted call")
    {
        TestEmitter emitter;
        Ptrs<TestReceiver> receivers_ptrs(5);

        uint32_t receiver_index = 0;
        for(Ptr<TestReceiver>& receiver_ptr : receivers_ptrs)
        {
            receiver_ptr = std::make_shared<TestReceiver>(receiver_index++);
            receiver_ptr->CheckBind(emitter);
        }

        CHECK_NOTHROW(emitter.EmitCall([&receivers_ptrs](uint32_t receiver_index)
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

        CHECK_FALSE(receiver.IsFooCalled());
        CHECK_FALSE(receiver.IsBarCalled());

        uint32_t emit_count = 0U;
        for(TestEmitter& emitter : emitters)
        {
            CHECK_NOTHROW(emitter.EmitFoo());

            emit_count++;
            CHECK(receiver.GetFooCallCount() == emit_count);
        }

        CHECK_FALSE(receiver.IsBarCalled());
    }

    SECTION("Emit with arguments")
    {
        std::array<TestEmitter, 5> emitters;
        TestReceiver receiver;

        for(TestEmitter& emitter : emitters)
        {
            receiver.CheckBind(emitter);
        }

        CHECK_FALSE(receiver.IsFooCalled());
        CHECK_FALSE(receiver.IsBarCalled());
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

        CHECK_FALSE(receiver.IsFooCalled());
    }

    SECTION("Copied emitters are connected to receiver")
    {
        TestEmitter emitter;
        TestReceiver receiver;
        receiver.CheckBind(emitter);

        std::vector<TestEmitter> emitter_copies;
        for(size_t id = 0; id < 5; ++id)
        {
            CHECK_NOTHROW(emitter_copies.push_back(emitter));
            CHECK(emitter_copies.back().GetConnectedReceiversCount() == 1);
        }
        CHECK(receiver.GetConnectedEmittersCount() == emitter_copies.size() + 1);

        uint32_t foo_call_count = 1;
        CHECK_NOTHROW(emitter.EmitFoo());
        CHECK(receiver.GetFooCallCount() == foo_call_count++);

        for(TestEmitter& emitter_copy : emitter_copies)
        {
            CHECK_NOTHROW(emitter_copy.EmitFoo());
            CHECK(receiver.GetFooCallCount() == foo_call_count++);
        }

        emitter_copies.clear();
        CHECK(receiver.GetConnectedEmittersCount() == 1);
    }

    SECTION("Connect emitters during emitted call")
    {
        std::array<TestEmitter, 5> emitters;
        TestReceiver receiver;

        for(TestEmitter& emitter : emitters)
        {
            receiver.CheckBind(emitter);
        }

        CHECK(receiver.GetConnectedEmittersCount() == emitters.size());
        Ptrs<TestEmitter> dynamic_emitters;

        for(TestEmitter& emitter : emitters)
        {
            CHECK_NOTHROW(emitter.EmitCall([&dynamic_emitters, &receiver](size_t)
            {
                auto new_emitter_ptr = std::make_shared<TestEmitter>();
                receiver.CheckBind(*new_emitter_ptr);
                dynamic_emitters.emplace_back(std::move(new_emitter_ptr));
            }));
        }

        CHECK(receiver.GetFuncCallCount() == emitters.size());
        CHECK(dynamic_emitters.size() == emitters.size());
        CHECK(receiver.GetConnectedEmittersCount() == emitters.size() + dynamic_emitters.size());

        for(Ptr<TestEmitter>& emitter_ptr : dynamic_emitters)
        {
            emitter_ptr->EmitFoo();
        }
        CHECK(receiver.GetFooCallCount() == dynamic_emitters.size());

        dynamic_emitters.clear();
        CHECK(receiver.GetConnectedEmittersCount() == emitters.size());
    }

    SECTION("Destroy emitters during emitted call")
    {
        Ptrs<TestEmitter> emitters;
        TestReceiver      receiver;

        for (size_t id = 0; id < 6; ++id)
        {
            auto new_emitter_ptr = std::make_shared<TestEmitter>();
            receiver.CheckBind(*new_emitter_ptr);
            emitters.emplace_back(std::move(new_emitter_ptr));
        }
        CHECK(receiver.GetConnectedEmittersCount() == emitters.size());

        const size_t emits_count = 3;
        for (size_t id = 0; id < emits_count; ++id)
        {
            CHECK_NOTHROW(emitters[id]->EmitCall([&emitters](size_t)
            {
                emitters.pop_back();
            }));
        }

        CHECK(receiver.GetFuncCallCount() == emits_count);
        CHECK(receiver.GetConnectedEmittersCount() == emits_count);
    }
}

TEST_CASE("Connect emitter to receiver through the transmitter", "[events]")
{
    TestEmitter  emitter;
    TestReceiver receiver;

    SECTION("Emit Foo through transmitter connection")
    {
        TestTransmitter transmitter(emitter);
        CHECK_NOTHROW(transmitter.Connect(receiver));

        CHECK_FALSE(receiver.IsFooCalled());
        CHECK_FALSE(receiver.IsBarCalled());

        CHECK_NOTHROW(emitter.EmitFoo());

        CHECK(receiver.IsFooCalled());
        CHECK_FALSE(receiver.IsBarCalled());
    }

    SECTION("Emit Bar through transmitter connection")
    {
        TestTransmitter transmitter(emitter);
        CHECK_NOTHROW(transmitter.Connect(receiver));

        CHECK_FALSE(receiver.IsFooCalled());
        CHECK_FALSE(receiver.IsBarCalled());
        CHECK(receiver.GetBarA() == 0);
        CHECK(receiver.GetBarB() == false);
        CHECK(receiver.GetBarC() == 0.f);

        CHECK_NOTHROW(emitter.EmitBar(g_bar_a, g_bar_b, g_bar_c));

        CHECK_FALSE(receiver.IsFooCalled());
        CHECK(receiver.IsBarCalled());
        CHECK(receiver.GetBarA() == g_bar_a);
        CHECK(receiver.GetBarB() == g_bar_b);
        CHECK(receiver.GetBarC() == g_bar_c);
    }

    SECTION("Transmitter can disconnect from receiver")
    {
        TestTransmitter transmitter(emitter);
        CHECK_NOTHROW(transmitter.Connect(receiver));
        CHECK_NOTHROW(transmitter.Disconnect(receiver));
        CHECK_NOTHROW(emitter.EmitFoo());
        CHECK_FALSE(receiver.IsFooCalled());
    }

    SECTION("Transmitter can be reset to other emitter")
    {
        TestTransmitter transmitter(emitter);
        TestEmitter other_emitter;
        transmitter.Reset(&other_emitter);

        CHECK_NOTHROW(transmitter.Connect(receiver));
        CHECK_FALSE(receiver.IsFooCalled());
        CHECK_NOTHROW(emitter.EmitFoo());
        CHECK_FALSE(receiver.IsFooCalled());
        CHECK_NOTHROW(other_emitter.EmitFoo());
        CHECK(receiver.IsFooCalled());
    }

    SECTION("Connect/Disconnect through default constructed transmitter throws error")
    {
        TestTransmitter transmitter;
        CHECK_FALSE(transmitter.IsTransmitting());
        CHECK_THROWS_AS(transmitter.Connect(receiver), TestTransmitter::NoTargetError);
        CHECK_THROWS_AS(transmitter.Disconnect(receiver), TestTransmitter::NoTargetError);
    }

    SECTION("Connect/Disconnect through disconnected transmitter throws error")
    {
        TestTransmitter transmitter(emitter);
        CHECK(transmitter.IsTransmitting());
        transmitter.Reset();
        CHECK_FALSE(transmitter.IsTransmitting());
        CHECK_THROWS_AS(transmitter.Connect(receiver), TestTransmitter::NoTargetError);
        CHECK_THROWS_AS(transmitter.Disconnect(receiver), TestTransmitter::NoTargetError);
    }
}