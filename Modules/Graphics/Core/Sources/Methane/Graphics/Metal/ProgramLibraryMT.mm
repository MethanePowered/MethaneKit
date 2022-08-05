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

FILE: Methane/Graphics/Metal/ProgramLibraryMT.mm
Wrapper of the Metal program library.

******************************************************************************/

#include "ProgramLibraryMT.hh"
#include "DeviceMT.hh"

#include <Methane/Platform/Utils.h>
#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

static NSString* GetLibraryFullPath(const std::string& library_name)
{
    return MacOS::ConvertToNsType<std::string, NSString*>(Platform::GetResourceDir() + "/" + library_name + ".metallib");
}

ProgramLibraryMT::ProgramLibraryMT(const DeviceMT& metal_device, const std::string& library_name)
{
    META_FUNCTION_TASK();
    if (library_name.empty())
    {
        m_mtl_library = [metal_device.GetNativeDevice() newDefaultLibrary];
    }
    else
    {
        NSError* ns_error = nil;
        m_mtl_library = [metal_device.GetNativeDevice() newLibraryWithFile:GetLibraryFullPath(library_name) error:&ns_error];
        META_CHECK_ARG_NOT_NULL_DESCR(m_mtl_library,
                                      "Failed to create {} Metal library: {}",
                                      library_name.empty() ? std::string("default") : library_name,
                                      MacOS::ConvertFromNsType<NSString, std::string>([ns_error localizedDescription]));
    }
}

} // namespace Methane::Graphics
