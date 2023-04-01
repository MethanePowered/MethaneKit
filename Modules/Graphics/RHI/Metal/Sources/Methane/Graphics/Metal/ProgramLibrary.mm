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

FILE: Methane/Graphics/Metal/ProgramLibrary.mm
Wrapper of the Metal program library.

******************************************************************************/

#include <Methane/Graphics/Metal/ProgramLibrary.hh>
#include <Methane/Graphics/Metal/Device.hh>

#include <Methane/Platform/Utils.h>
#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>

namespace Methane::Graphics::Metal
{

static NSString* GetLibraryFullPath(std::string_view library_name)
{
    const std::string library_path = fmt::format("{}/{}.metallib", Platform::GetResourceDir(), library_name);
    return MacOS::ConvertToNsString(library_path);
}

ProgramLibrary::ProgramLibrary(const Device& metal_device, std::string_view library_name)
{
    META_FUNCTION_TASK();
    if (library_name.empty())
    {
        m_mtl_library = [metal_device.GetNativeDevice() newDefaultLibrary];
    }
    else
    {
        NSError* ns_error = nil;
        NSURL* library_url = [NSURL fileURLWithPath:GetLibraryFullPath(library_name)];
        m_mtl_library = [metal_device.GetNativeDevice() newLibraryWithURL:library_url error:&ns_error];
        META_CHECK_ARG_NOT_NULL_DESCR(m_mtl_library,
                                      "Failed to create {} Metal library: {}",
                                      library_name.empty() ? std::string_view("default") : library_name,
                                      MacOS::ConvertFromNsString([ns_error localizedDescription]));
    }
}

} // namespace Methane::Graphics::Metal
