/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/InstrumentMemoryAllocations.cpp
Overloading "new" and "delete" operators with additional instrumentation:
 - Memory allocations tracking with Tracy

******************************************************************************/

#include <Methane/InstrumentMemoryAllocations.h>

#include <Tracy.hpp>

void* operator new(std::size_t size)
{
    void* ptr = std::malloc(size);
    if (!ptr)
        throw std::bad_alloc();

    TracyAlloc(ptr, size)
    return ptr;
}

void* operator new(std::size_t size, std::align_val_t align)
{
#if defined(_WIN32)
    void* ptr = _aligned_malloc(size, static_cast<std::size_t>(align));
#elif defined(__APPLE__)
    void* ptr = nullptr;
    const int error = posix_memalign(&ptr, static_cast<size_t>(align), size);
    if (error)
        throw std::bad_alloc();
#else // Linux
    void* ptr = aligned_alloc(static_cast<std::size_t>(align), size);
#endif

    if (!ptr)
        throw std::bad_alloc{};

    TracyAlloc(ptr, size)
    return ptr;
}

void operator delete(void* ptr) throw()
{
    TracyFree(ptr)
    std::free(ptr);
}

void operator delete(void* ptr, size_t) throw()
{
    TracyFree(ptr)
    std::free(ptr);
}

void operator delete(void* ptr, std::align_val_t) throw()
{
    TracyFree(ptr)
#if defined(_WIN32)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}