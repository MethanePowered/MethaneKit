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

FILE: Methane/Graphics/Noise.cpp
Multi-octave simplex noise generator in range [0, 1]

******************************************************************************/

#include <Methane/Graphics/PerlinNoise.h>

#include <simplexnoise1234.h>

namespace Methane::Graphics
{

PerlinNoise::PerlinNoise(float persistence, size_t octaves_count)
    : m_weights(GetWeights(persistence, octaves_count))
    , m_norm_multiplier(0.5f / GetWeightsSum(m_weights))
{ }

float PerlinNoise::operator()(Vector2f pos) const
{
    float noise = 0.0f;
    for (const float weight : m_weights)
    {
        noise += weight * SimplexNoise1234::noise(pos[0], pos[1]);
        pos *= 2.f;
    }
    return noise * m_norm_multiplier + 0.5f;
}

float PerlinNoise::operator()(Vector3f pos) const
{
    float noise = 0.0f;
    for (const float weight : m_weights)
    {
        noise += weight * SimplexNoise1234::noise(pos[0], pos[1], pos[2]);
        pos *= 2.f;
    }
    return noise * m_norm_multiplier + 0.5f;
}

float PerlinNoise::operator()(Vector4f pos) const
{
    float noise = 0.f;
    for (const float weight : m_weights)
    {
        noise += weight * SimplexNoise1234::noise(pos[0], pos[1], pos[2], pos[3]);
        pos *= 2.f;
    }
    return noise * m_norm_multiplier + 0.5f;
}

PerlinNoise::Weights PerlinNoise::GetWeights(float persistence, size_t octaves_count)
{
    Weights weights;
    weights.reserve(octaves_count);
    for (size_t i = 0; i < octaves_count; ++i)
    {
        weights.emplace_back(persistence);
        persistence *= persistence;
    }
    return weights;
}

float PerlinNoise::GetWeightsSum(const PerlinNoise::Weights& weights)
{
    float weights_sum = 0.f;
    for(float weight : weights)
    {
        weights_sum += weight;
    }
    return weights_sum;
}

} // namespace Methane::Graphics
