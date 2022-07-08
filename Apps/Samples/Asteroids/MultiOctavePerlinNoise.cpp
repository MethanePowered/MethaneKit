/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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

FILE: MultiOctavePerlinNoise.cpp
Multi-octave simplex noise generator in range [0, 1]

******************************************************************************/

#include "MultiOctavePerlinNoise.h"
#include <Methane/Instrumentation.h>

#include <hlsl++.h>
#include <FastNoise/FastNoise.h>

namespace Methane::Samples
{

template<typename VectorType>
float GetPerlinNoise(const FastNoise::Simplex& simplex_noise, const VectorType& pos, int seed) noexcept;

template<> float GetPerlinNoise(const FastNoise::Simplex& simplex_noise, const hlslpp::float2& pos, int seed) noexcept    { return simplex_noise.GenSingle2D(pos.x, pos.y, seed); }
template<> float GetPerlinNoise(const FastNoise::Simplex& simplex_noise, const hlslpp::float3& pos, int seed) noexcept    { return simplex_noise.GenSingle3D(pos.x, pos.y, pos.z, seed); }
template<> float GetPerlinNoise(const FastNoise::Simplex& simplex_noise, const hlslpp::float4& pos, int seed) noexcept    { return simplex_noise.GenSingle4D(pos.x, pos.y, pos.z, pos.w, seed); }

template<> float GetPerlinNoise(const FastNoise::Simplex& simplex_noise, const Data::RawVector2F& pos, int seed) noexcept { return simplex_noise.GenSingle2D(pos[0], pos[1], seed); }
template<> float GetPerlinNoise(const FastNoise::Simplex& simplex_noise, const Data::RawVector3F& pos, int seed) noexcept { return simplex_noise.GenSingle3D(pos[0], pos[1], pos[2], seed); }
template<> float GetPerlinNoise(const FastNoise::Simplex& simplex_noise, const Data::RawVector4F& pos, int seed) noexcept { return simplex_noise.GenSingle4D(pos[0], pos[1], pos[2], pos[3], seed); }

MultiOctavePerlinNoise::MultiOctavePerlinNoise(float persistence, size_t octaves_count, int seed)
    : m_weights(GetWeights(persistence, octaves_count))
    , m_norm_multiplier(0.5F / GetWeightsSum(m_weights))
    , m_seed(seed)
{
    META_FUNCTION_TASK();
}

float MultiOctavePerlinNoise::operator()(const hlslpp::float2& pos) const noexcept { return GetValue(pos); }
float MultiOctavePerlinNoise::operator()(const hlslpp::float3& pos) const noexcept { return GetValue(pos); }
float MultiOctavePerlinNoise::operator()(const hlslpp::float4& pos) const noexcept { return GetValue(pos); }

float MultiOctavePerlinNoise::operator()(const Data::RawVector2F& pos) const noexcept { return GetValue(pos); }
float MultiOctavePerlinNoise::operator()(const Data::RawVector3F& pos) const noexcept { return GetValue(pos); }
float MultiOctavePerlinNoise::operator()(const Data::RawVector4F& pos) const noexcept { return GetValue(pos); }

template<typename VectorType>
float MultiOctavePerlinNoise::GetValue(VectorType pos) const noexcept
{
    META_FUNCTION_TASK();
    const FastNoise::Simplex& simplex_noise = GetSimplexNoise();
    float noise = 0.F;
    for (const float weight : m_weights)
    {
        noise += weight * GetPerlinNoise(simplex_noise, pos, m_seed);
        pos *= 2.F;
    }
    return noise * m_norm_multiplier + 0.5F;
}

MultiOctavePerlinNoise::Weights MultiOctavePerlinNoise::GetWeights(float persistence, size_t octaves_count) noexcept
{
    META_FUNCTION_TASK();
    Weights weights;
    weights.reserve(octaves_count);
    for (size_t i = 0; i < octaves_count; ++i)
    {
        weights.emplace_back(persistence);
        persistence *= persistence;
    }
    return weights;
}

float MultiOctavePerlinNoise::GetWeightsSum(const MultiOctavePerlinNoise::Weights& weights) noexcept
{
    META_FUNCTION_TASK();
    float weights_sum = 0.F;
    for(float weight : weights)
    {
        weights_sum += weight;
    }
    return weights_sum;
}

const FastNoise::Simplex& MultiOctavePerlinNoise::GetSimplexNoise() const
{
    META_FUNCTION_TASK();
    static const auto s_simplex_noise_ptr = FastNoise::New<FastNoise::Simplex>();
    return *s_simplex_noise_ptr;
}

} // namespace Methane::Graphics
