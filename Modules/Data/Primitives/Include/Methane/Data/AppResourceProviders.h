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

FILE: Methane/Graphics/AppResourceProviders.h
Application resource providers for Shaders and Textures.

******************************************************************************/

#pragma once

#ifdef SHADER_RESOURCE_NAMESPACE

#ifdef RESOURCE_NAMESPACE
#undef RESOURCE_NAMESPACE
#endif

#define RESOURCE_NAMESPACE SHADER_RESOURCE_NAMESPACE
#include "ResourceProvider.hpp"

namespace Methane::Data
{
using ShaderProvider = SHADER_RESOURCE_NAMESPACE::ResourceProvider;
}

#else

#include "FileProvider.hpp"

namespace Methane::Data
{
    using ShaderProvider = FileProvider;
}

#endif


#ifdef TEXTURE_RESOURCE_NAMESPACE

#ifdef RESOURCE_NAMESPACE
#undef RESOURCE_NAMESPACE
#endif

#define RESOURCE_NAMESPACE TEXTURE_RESOURCE_NAMESPACE
#include "ResourceProvider.hpp"

namespace Methane::Data
{
using TextureProvider = TEXTURE_RESOURCE_NAMESPACE::ResourceProvider;
}

#else

#include "FileProvider.hpp"

namespace Methane::Data
{
    using TextureProvider = FileProvider;
}

#endif