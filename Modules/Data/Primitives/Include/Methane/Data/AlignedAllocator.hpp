/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Data/AlignedAllocator.hpp
Aligned memory allocator to be used in STL containers, like std::vector.

******************************************************************************/

#pragma once

#include <stdlib.h>
#include <malloc.h>

namespace Methane::Data
{

template <typename T, size_t N = 16>
class AlignedAllocator
{
public:
    using value_type = T;
    using size_type = std::size_t    ;
    using difference_type =  std::ptrdiff_t;

    using pointer = T*;
    using const_pointer = const T*;

    using reference = T&       ;
    using const_reference = const T& ;

    AlignedAllocator() = default;

    template <typename T2>
    AlignedAllocator(const AlignedAllocator<T2, N>&) noexcept { }

    static pointer address(reference r)             { return &r; }
    static const_pointer address(const_reference r) { return &r; }

    static pointer allocate(size_type n)
    {
#ifdef WIN32
        return static_cast<pointer>(_aligned_malloc(n * sizeof(value_type), N));
#else
        void* p_memory = nullptr;
        posix_memalign(&p_memory, n * sizeof(value_type), N);
        return p_memory;
#endif
    }

    void deallocate(pointer p, size_type)
    {
#ifdef WIN32
        _aligned_free(p);
#else
        free(p);
#endif
    }

    void construct(pointer p, const value_type& value)
    {
        new(p) value_type(value);
    }

    void destroy(pointer p)
    {
        p->~value_type();
    }

    size_type max_size() const noexcept
    {
        return size_type(-1) / sizeof(value_type);
    }

    template <typename T2>
    struct rebind
    {
        typedef AlignedAllocator<T2, N> other;
    };

    bool operator!=(const AlignedAllocator<T, N>& other) const
    {
        return !(*this == other);
    }

    bool operator==(const AlignedAllocator<T, N>& other) const
    {
        return true;
    }
};

} // namespace Methane::Data
