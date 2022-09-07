/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/MacOS/Instrumentation.mm
MacOS implementation of the platform specific instrumentation functions.

******************************************************************************/

#include <pthread.h>
#include <string_view>

#import <Foundation/Foundation.h>

namespace Methane
{

void SetThreadName(std::string_view name)
{
    NSString* ns_name = [[NSString alloc] initWithBytes:name.data()
                                                 length:name.length()
                                               encoding:NSUTF8StringEncoding];
    [[NSThread currentThread] setName:ns_name];
    pthread_setname_np(name.data());
}

} // namespace Methane