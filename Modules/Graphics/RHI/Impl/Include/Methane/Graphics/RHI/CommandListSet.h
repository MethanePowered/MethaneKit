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

FILE: Methane/Graphics/RHI/CommandListSet.h
Methane CommandListSet PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/ICommandListSet.h>

namespace Methane::Graphics::META_GFX_NAME
{
class CommandListSet;
}

namespace Methane::Graphics::Rhi
{

class CommandListSet // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(CommandListSet);
    META_PIMPL_METHODS_COMPARE_INLINE(CommandListSet);

    META_PIMPL_API explicit CommandListSet(const Ptr<ICommandListSet>& interface_ptr);
    META_PIMPL_API explicit CommandListSet(ICommandListSet& interface_ref);
    META_PIMPL_API CommandListSet(const Refs<ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt = {});

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API ICommandListSet& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<ICommandListSet> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // ICommandListSet interface methods
    [[nodiscard]] META_PIMPL_API Data::Size                GetCount() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API const Refs<ICommandList>& GetRefs() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API ICommandList&             operator[](Data::Index index) const;
    [[nodiscard]] META_PIMPL_API const Opt<Data::Index>&   GetFrameIndex() const META_PIMPL_NOEXCEPT;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::CommandListSet;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/CommandListSet.cpp>

#endif // META_PIMPL_INLINE
