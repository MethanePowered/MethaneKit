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

FILE: Methane/Instrumentation.cpp
Common header for instrumentation of the Methane Kit modules with ITT macroses,
Defines common ITT domain required for instrumentation.

******************************************************************************/

#include <Methane/Instrumentation.h>

ITT_DOMAIN_GLOBAL(METHANE_DOMAIN_NAME);

namespace Methane::ITT
{

#ifdef ITT_INSTRUMENTATION_ENABLED

template<> __itt_metadata_type Counter<double>::GetValueType()   noexcept { return __itt_metadata_double; }
template<> __itt_metadata_type Counter<float>::GetValueType()    noexcept { return __itt_metadata_float;  }
template<> __itt_metadata_type Counter<int16_t>::GetValueType()  noexcept { return __itt_metadata_s16;    }
template<> __itt_metadata_type Counter<uint16_t>::GetValueType() noexcept { return __itt_metadata_u16;    }
template<> __itt_metadata_type Counter<int32_t>::GetValueType()  noexcept { return __itt_metadata_s32;    }
template<> __itt_metadata_type Counter<uint32_t>::GetValueType() noexcept { return __itt_metadata_u32;    }
template<> __itt_metadata_type Counter<int64_t>::GetValueType()  noexcept { return __itt_metadata_s64;    }
template<> __itt_metadata_type Counter<uint64_t>::GetValueType() noexcept { return __itt_metadata_u64;    }

#endif

} // namespace Methane::ITT