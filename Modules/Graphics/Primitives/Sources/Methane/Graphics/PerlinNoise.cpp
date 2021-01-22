/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Noise.cpp
Multi-octave simplex noise generator in range [0, 1]

******************************************************************************/

#include <Methane/Graphics/PerlinNoise.h>
#include <Methane/Instrumentation.h>

#include <hlsl++.h>
#include <simplexnoise1234.h>

namespace Methane::Graphics
{

template<typename VectorType>
float GetPerlinNoise(const VectorType& pos) noexcept;

template<> float GetPerlinNoise(const hlslpp::float2& pos) noexcept { return SimplexNoise1234::noise(pos.x, pos.y); }
template<> float GetPerlinNoise(const hlslpp::float3& pos) noexcept { return SimplexNoise1234::noise(pos.x, pos.y, pos.z); }
template<> float GetPerlinNoise(const hlslpp::float4& pos) noexcept { return SimplexNoise1234::noise(pos.x, pos.y, pos.z, pos.w); }

template<> float GetPerlinNoise(const Data::RawVector2F& pos) noexcept { return SimplexNoise1234::noise(pos[0], pos[1]); }
template<> float GetPerlinNoise(const Data::RawVector3F& pos) noexcept { return SimplexNoise1234::noise(pos[0], pos[1], pos[2]); }
template<> float GetPerlinNoise(const Data::RawVector4F& pos) noexcept { return SimplexNoise1234::noise(pos[0], pos[1], pos[2], pos[3]); }

PerlinNoise::PerlinNoise(float persistence, size_t octaves_count)
    : m_weights(GetWeights(persistence, octaves_count))
    , m_norm_multiplier(0.5F / GetWeightsSum(m_weights))
{
    META_FUNCTION_TASK();
}

float PerlinNoise::operator()(const hlslpp::float2& pos) const noexcept { return GetValue(pos); }
float PerlinNoise::operator()(const hlslpp::float3& pos) const noexcept { return GetValue(pos); }
float PerlinNoise::operator()(const hlslpp::float4& pos) const noexcept { return GetValue(pos); }

float PerlinNoise::operator()(const Data::RawVector2F& pos) const noexcept { return GetValue(pos); }
float PerlinNoise::operator()(const Data::RawVector3F& pos) const noexcept { return GetValue(pos); }
float PerlinNoise::operator()(const Data::RawVector4F& pos) const noexcept { return GetValue(pos); }

template<typename VectorType>
float PerlinNoise::GetValue(VectorType pos) const noexcept
{
    META_FUNCTION_TASK();
    float noise = 0.F;
    for (const float weight : m_weights)
    {
        noise += weight * GetPerlinNoise(pos);
        pos *= 2.F;
    }
    return noise * m_norm_multiplier + 0.5F;
}

PerlinNoise::Weights PerlinNoise::GetWeights(float persistence, size_t octaves_count) noexcept
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

float PerlinNoise::GetWeightsSum(const PerlinNoise::Weights& weights) noexcept
{
    META_FUNCTION_TASK();
    float weights_sum = 0.F;
    for(float weight : weights)
    {
        weights_sum += weight;
    }
    return weights_sum;
}

} // namespace Methane::Graphics
