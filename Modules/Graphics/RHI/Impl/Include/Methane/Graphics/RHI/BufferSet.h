/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/BufferSet.h
Methane BufferSet PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/IBufferSet.h>

#include <vector>

namespace Methane::Graphics::META_GFX_NAME
{
class BufferSet;
}

namespace Methane::Graphics::Rhi
{

class Buffer;

class BufferSet // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using Buffers = std::vector<Buffer>;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(BufferSet);
    META_PIMPL_METHODS_COMPARE_INLINE(BufferSet);

    META_PIMPL_API explicit BufferSet(const Ptr<IBufferSet>& interface_ptr);
    META_PIMPL_API explicit BufferSet(IBufferSet& interface_ref);
    META_PIMPL_API BufferSet(BufferType buffers_type, const Refs<Buffer>& buffer_refs);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API IBufferSet& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<IBufferSet> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_PIMPL_API bool SetName(std::string_view name) const;
    META_PIMPL_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IBufferSet interface methods
    [[nodiscard]] META_PIMPL_API BufferType     GetType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API Data::Size     GetCount() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API const Buffers& GetRefs() const noexcept;
    [[nodiscard]] META_PIMPL_API std::string    GetNames() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API const Buffer&  operator[](Data::Index index) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::BufferSet;

    Ptr<Impl> m_impl_ptr;
    mutable Buffers m_buffers;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/BufferSet.cpp>

#endif // META_PIMPL_INLINE
