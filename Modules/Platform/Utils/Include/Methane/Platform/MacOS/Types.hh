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

FILE: Methane/Platform/MacOS/Types.h
MacOS platform type convertors.

******************************************************************************/

#pragma once

#include <string>
#include <vector>

#import <Cocoa/Cocoa.h>

namespace Methane
{
namespace MacOS
{

// ===============================
// Conversion from STL to NS types
// ===============================

template<class STDType, class NSType>
inline NSType ConvertToNSType(const STDType& value);

template<>
inline BOOL ConvertToNSType<bool, BOOL>(const bool& value)
{
    return value ? YES : NO;
}

template<>
inline NSString* ConvertToNSType<std::string, NSString*>(const std::string& str)
{
    return [[NSString alloc] initWithUTF8String:str.data()];
}

template<typename STDItemType, typename NSItemType>
inline NSMutableArray<NSItemType>* ConvertToNSList(const std::vector<STDItemType>& std_vector)
{
    NSMutableArray<NSItemType>* p_mutable_array = [[NSMutableArray<NSItemType> alloc] init];
    if (!p_mutable_array)
        return nil;

    for(const STDItemType& std_item : std_vector)
    {
        NSItemType p_ns_item = ConvertToNSType<STDItemType, NSItemType>(std_item);
        if (!p_ns_item)
            continue;

        [p_mutable_array addObject:p_ns_item];
    }

    return p_mutable_array;
}

// ===============================
// Conversion from NS to QT types
// ===============================

template<class NSType, class STDType>
inline STDType ConvertFromNSType(const NSType& value);

template<class NSType, class STDType>
inline STDType ConvertFromNSType(const NSType* p_ns_value);

template<>
inline bool ConvertFromNSType<BOOL, bool>(const BOOL& value)
{
    return value == YES;
}

template<>
inline std::string ConvertFromNSType<NSString, std::string>(const NSString* p_ns_str)
{
    return p_ns_str ? std::string([p_ns_str UTF8String]) : std::string();
}

} // namespace MacOS
} // namespace Methane
