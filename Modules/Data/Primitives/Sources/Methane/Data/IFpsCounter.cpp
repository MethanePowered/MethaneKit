/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/IFpsCounter.cpp
FPS counter interface.

******************************************************************************/

#include <Methane/Data/IFpsCounter.h>

#include <Methane/Instrumentation.h>

namespace Methane::Data
{

FrameTiming::FrameTiming(double total_time_sec, double present_time_sec, double gpu_wait_time_sec) noexcept
    : m_total_time_sec(total_time_sec)
    , m_present_time_sec(present_time_sec)
    , m_gpu_wait_time_sec(gpu_wait_time_sec)
{ }

FrameTiming& FrameTiming::operator+=(const FrameTiming& other) noexcept
{
    META_FUNCTION_TASK();
    m_total_time_sec    += other.m_total_time_sec;
    m_present_time_sec  += other.m_present_time_sec;
    m_gpu_wait_time_sec += other.m_gpu_wait_time_sec;
    return *this;
}

FrameTiming& FrameTiming::operator-=(const FrameTiming& other) noexcept
{
    META_FUNCTION_TASK();
    m_total_time_sec    -= other.m_total_time_sec;
    m_present_time_sec  -= other.m_present_time_sec;
    m_gpu_wait_time_sec -= other.m_gpu_wait_time_sec;
    return *this;
}

} // namespace Methane::Graphics::Rhi
