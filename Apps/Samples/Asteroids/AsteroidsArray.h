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

FILE: AsteroidsArray.h
Random generated asteroids array with uber mesh and textures ready for rendering.

******************************************************************************/

#pragma once

#include "Asteroid.h"

#include <Methane/Graphics/Context.h>
#include <Methane/Graphics/Mesh.h>
#include <Methane/Graphics/MeshBuffers.hpp>
#include <Methane/Graphics/Camera.h>

namespace Methane::Samples
{

namespace gfx = Graphics;

class AsteroidArray : public gfx::TexturedMeshBuffers<AsteroidUniforms>
{
public:
    using Ptr = std::unique_ptr<AsteroidArray>;
    using BaseBuffers = gfx::TexturedMeshBuffers<AsteroidUniforms>;

    struct Settings
    {
        const gfx::Camera&  view_camera;
        const uint32_t      instance_count      = 3;
        const uint32_t      subdivisions_count  = 3;
        float               scale               = 1.f;
        const uint32_t      random_seed         = 1337;
    };

    class UberMesh : public gfx::UberMesh<Asteroid::Vertex>
    {
    public:
        UberMesh(uint32_t instance_count, uint32_t subdivisions_count, uint32_t random_seed);
    };

    AsteroidArray(gfx::Context& context, Settings settings);

    bool Update(double elapsed_seconds, double delta_seconds);

private:
    using Parameters = std::vector<Asteroid::Parameters>;

    Settings   m_settings;
    Parameters m_parameters;
};

} // namespace Methane::Samples
