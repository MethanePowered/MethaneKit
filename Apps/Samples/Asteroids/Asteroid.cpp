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

FILE: Asteroid.cpp
Random generated asteroid model with mesh and texture ready for rendering

******************************************************************************/

#include "Asteroid.h"

#include <Methane/Graphics/Noise.hpp>
#include <Methane/Data/Parallel.hpp>

namespace Methane::Samples
{

Asteroid::Mesh::Mesh(uint32_t subdivisions_count, bool randomize)
    : gfx::IcosahedronMesh<Vertex>(VertexLayoutFromArray(Vertex::layout), 0.5f, subdivisions_count, true)
{
    if (randomize)
    {
        Randomize();
    }
}

void Asteroid::Mesh::Randomize(uint32_t random_seed)
{
    const float noise_scale = 0.5f;
    const float radius_scale = 1.8f;
    const float radius_bias = 0.3f;

    std::mt19937 rng(random_seed);

    auto random_persistence = std::normal_distribution<float>(0.95f, 0.04f);
    const gfx::NoiseOctaves<4> perlin_noise(random_persistence(rng));

    auto  random_noise = std::uniform_real_distribution<float>(0.0f, 10000.0f);
    const float noise = random_noise(rng);

    for (Vertex& vertex : m_vertices)
    {
        vertex.position *= perlin_noise(gfx::Vector4f(vertex.position * noise_scale, noise)) * radius_scale + radius_bias;
    }

    ComputeAverageNormals();
}

Asteroid::Asteroid(gfx::Context& context)
    : BaseBuffers(context, Mesh(3, true), "Asteroid")
{
}


AsteroidArray::UberMesh::UberMesh(uint32_t instance_count, uint32_t subdivisions_count, uint32_t random_seed)
    : gfx::UberMesh<Asteroid::Vertex>(gfx::Mesh::VertexLayoutFromArray(Asteroid::Vertex::layout))
{
    std::mt19937 rng(random_seed);

    for(uint32_t instance_index = 0; instance_index < instance_count; ++instance_index)
    {
        const uint32_t instance_seed = rng();
        Asteroid::Mesh base_mesh(0, false);

        for (uint32_t subdivision_index = 0; subdivision_index < subdivisions_count; ++subdivision_index)
        {
            base_mesh.Subdivide();

            Asteroid::Mesh asteroid_mesh(base_mesh);
            asteroid_mesh.Spherify();
            asteroid_mesh.Randomize(instance_seed);

            AddSubMesh(asteroid_mesh, true);
        }
    }
}

AsteroidArray::AsteroidArray(gfx::Context& context, uint32_t instance_count, uint32_t subdivisions_count, uint32_t random_seed)
    : BaseBuffers(context, UberMesh(instance_count, subdivisions_count, random_seed), "Asteroid Array")
{
}

} // namespace Methane::Samples
