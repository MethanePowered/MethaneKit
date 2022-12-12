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

FILE: Methane/Platform/MacOS/Types.h
MacOS platform type converters.

******************************************************************************/

#pragma once

#import <Foundation/NSString.h>

#include <string>

namespace Methane::MacOS
{

// ===============================
// Conversion from STL to NS types
// ===============================

inline BOOL ConvertToNsBool(bool flag)
{
    return flag ? YES : NO;
}

inline NSString* ConvertToNsString(std::string_view str)
{
    return [[NSString alloc] initWithUTF8String:str.data()];
}

// ===============================
// Conversion from NS to QT types
// ===============================

inline bool ConvertFromNsBool(BOOL value)
{
    return value == YES;
}

inline std::string ConvertFromNsString(const NSString* p_ns_str)
{
    return p_ns_str ? std::string([p_ns_str UTF8String]) : std::string();
}

} // namespace Methane::MacOS
