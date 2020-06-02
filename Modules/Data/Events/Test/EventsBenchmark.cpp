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

FILE: Test/EventsBenchmark.cpp
Benchmark connection and emit of events with emitter and receiver classes.

******************************************************************************/

// Catch benchmarks do not work correctly on Windows with unknown reason...
// File compilation can not be disabled in CMake because of ParseAndAddCatchTests error on expression
#if !defined(WIN32) && defined(CATCH_CONFIG_ENABLE_BENCHMARKING)

#include "EventWrappers.hpp"

#include <catch2/catch.hpp>

using namespace Methane::Data;

static uint32_t MeasureEmitToManyReceivers(uint32_t receivers_count, Catch::Benchmark::Chronometer meter)
{
    TestEmitter emitter;
    std::vector<TestReceiver> receivers(receivers_count);

    for(TestReceiver& receiver : receivers)
    {
        receiver.Bind(emitter);
    }

    meter.measure([&]()
    {
        emitter.EmitBar(g_bar_a, g_bar_b, g_bar_c);
    });

    // Prevent code removal by optimizer and check received calls count
    uint32_t received_calls_count = 0u;
    for(TestReceiver& receiver : receivers)
    {
        received_calls_count += receiver.GetBarCallCount();
    }
    CHECK(received_calls_count == receivers_count * meter.runs());
    return received_calls_count;
}

static uint32_t MeasureConnectAndEmitToManyReceivers(uint32_t receivers_count, Catch::Benchmark::Chronometer meter)
{
    std::vector<TestReceiver> receivers(receivers_count);

    meter.measure([&]()
    {
        TestEmitter emitter;
        for(TestReceiver& receiver : receivers)
        {
            receiver.Bind(emitter);
        }
        emitter.EmitBar(g_bar_a, g_bar_b, g_bar_c);
    });

    // Prevent code removal by optimizer and check received calls count
    uint32_t received_calls_count = 0u;
    for(TestReceiver& receiver : receivers)
    {
        received_calls_count += receiver.GetBarCallCount();
    }
    CHECK(received_calls_count == (receivers_count * meter.runs()));
    return received_calls_count;
}

static uint32_t MeasureReceiveFromManyEmitters(uint32_t emitters_count, Catch::Benchmark::Chronometer meter)
{
    std::vector<TestEmitter> emitters(emitters_count);
    TestReceiver receiver;

    for (TestEmitter& emitter : emitters)
    {
        receiver.Bind(emitter);
    }

    meter.measure([&]()
    {
        for (TestEmitter& emitter : emitters)
        {
            emitter.EmitBar(g_bar_a, g_bar_b, g_bar_c);
        }
    });

    // Prevent code removal by optimizer and check received calls count
    CHECK(receiver.GetBarCallCount() == emitters_count * meter.runs());
    return receiver.GetBarCallCount();
}

static uint32_t MeasureConnectAndReceiveFromManyEmitters(uint32_t emitters_count, Catch::Benchmark::Chronometer meter)
{
    std::vector<TestEmitter> emitters(emitters_count);
    uint32_t received_calls_count = 0u;

    meter.measure([&]()
    {
        TestReceiver receiver;
        for (TestEmitter& emitter : emitters)
        {
            receiver.Bind(emitter);
        }
        for (TestEmitter& emitter : emitters)
        {
            emitter.EmitBar(g_bar_a, g_bar_b, g_bar_c);
        }
        received_calls_count += receiver.GetBarCallCount();
    });

    // Prevent code removal by optimizer and check received calls count
    CHECK(received_calls_count == emitters_count * meter.runs());
    return received_calls_count;
}

TEST_CASE("Benchmark connect and emit events", "[events][benchmark]")
{

    SECTION("Emit to many receivers")
    {
        BENCHMARK_ADVANCED("Emit to 10 receivers")(Catch::Benchmark::Chronometer meter)
        {
            return MeasureEmitToManyReceivers(10, meter);
        };
        BENCHMARK_ADVANCED("Emit to 100 receivers")(Catch::Benchmark::Chronometer meter)
        {
            return MeasureEmitToManyReceivers(100, meter);
        };
        BENCHMARK_ADVANCED("Emit to 1000 receivers")(Catch::Benchmark::Chronometer meter)
        {
            return MeasureEmitToManyReceivers(1000, meter);
        };
    }

    SECTION("Connect and emit to many receivers")
    {
        BENCHMARK_ADVANCED("Connect and emit to 10 receivers")(Catch::Benchmark::Chronometer meter)
        {
            return MeasureConnectAndEmitToManyReceivers(10, meter);
        };
        BENCHMARK_ADVANCED("Connect and emit to 100 receivers")(Catch::Benchmark::Chronometer meter)
        {
            return MeasureConnectAndEmitToManyReceivers(100, meter);
        };
        BENCHMARK_ADVANCED("Connect and emit to 1000 receivers")(Catch::Benchmark::Chronometer meter)
        {
            return MeasureConnectAndEmitToManyReceivers(1000, meter);
        };
    }

    SECTION("Receive from many emitters")
    {
        BENCHMARK_ADVANCED("Receive from 10 emitters")(Catch::Benchmark::Chronometer meter)
        {
            return MeasureReceiveFromManyEmitters(10, meter);
        };
        BENCHMARK_ADVANCED("Receive from 100 emitters")(Catch::Benchmark::Chronometer meter)
        {
            return MeasureReceiveFromManyEmitters(100, meter);
        };
        BENCHMARK_ADVANCED("Receive from 1000 emitters")(Catch::Benchmark::Chronometer meter)
        {
            return MeasureReceiveFromManyEmitters(1000, meter);
        };
    }

    SECTION("Connect and receive from many emitters")
    {
        BENCHMARK_ADVANCED("Connect and receive from 10 emitters")(Catch::Benchmark::Chronometer meter)
        {
            return MeasureConnectAndReceiveFromManyEmitters(10, meter);
        };
        BENCHMARK_ADVANCED("Connect and receive from 100 emitters")(Catch::Benchmark::Chronometer meter)
        {
            return MeasureConnectAndReceiveFromManyEmitters(100, meter);
        };
        BENCHMARK_ADVANCED("Connect and receive from 1000 emitters")(Catch::Benchmark::Chronometer meter)
        {
            return MeasureConnectAndReceiveFromManyEmitters(1000, meter);
        };
    }
}

#endif