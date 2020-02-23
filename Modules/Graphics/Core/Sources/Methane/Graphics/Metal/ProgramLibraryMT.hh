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

FILE: Methane/Graphics/Metal/ProgramLibraryMT.hh
Wrapper of the Metal program library.

******************************************************************************/

#pragma once

#import <Metal/Metal.h>

#include <string>

namespace Methane::Graphics
{

class DeviceMT;

class ProgramLibraryMT final
{
public:
    ProgramLibraryMT(DeviceMT& metal_device, const std::string& library_name = "");
    ~ProgramLibraryMT();

    id<MTLLibrary>& Get() noexcept { return m_mtl_library; }

private:
    NSError*       m_ns_error = nil;
    id<MTLLibrary> m_mtl_library;
};

} // namespace Methane::Graphics
