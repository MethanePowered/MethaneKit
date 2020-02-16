/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Version.h
Methane version macro definitions

******************************************************************************/

#pragma once

#ifndef METHANE_VERSION_MAJOR
#define METHANE_VERSION_MAJOR 0
#endif

#ifndef METHANE_VERSION_MINOR
#define METHANE_VERSION_MINOR 0
#endif

#ifndef METHANE_VERSION_BUILD
#define METHANE_VERSION_BUILD 0
#endif

#define VAL_TO_STR_HELPER(VALUE) #VALUE
#define VAL_TO_STR(VALUE) VAL_TO_STR_HELPER(VALUE)

#define METHANE_VERSION_MAJOR_STR VAL_TO_STR(METHANE_VERSION_MAJOR)
#define METHANE_VERSION_MINOR_STR VAL_TO_STR(METHANE_VERSION_MINOR)
#define METHANE_VERSION_BUILD_STR VAL_TO_STR(METHANE_VERSION_BUILD)
#define METHANE_VERSION_STR METHANE_VERSION_MAJOR_STR "." METHANE_VERSION_MINOR_STR "." METHANE_VERSION_BUILD_STR