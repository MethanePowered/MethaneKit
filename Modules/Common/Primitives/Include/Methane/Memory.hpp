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

FILE: Methane/Memory.hpp
Methane memory handling smart pointers and references.

******************************************************************************/

#pragma once

#include <memory>
#include <functional>
#include <vector>

namespace Methane
{

template<class T>
using Ptr = std::shared_ptr<T>;

template<class T>
using Ptrs = std::vector<Ptr<T>>;

template<class T>
using WeakPtr = std::weak_ptr<T>;

template<class T>
using WeakPtrs = std::vector<WeakPtr<T>>;

template<class T>
using UniquePtr = std::unique_ptr<T>;

template<class T>
using UniquePtrs = std::vector<UniquePtr<T>>;

template<class T>
using Ref = std::reference_wrapper<T>;

template<class T>
using Refs = std::vector<Ref<T>>;

} // namespace Methane