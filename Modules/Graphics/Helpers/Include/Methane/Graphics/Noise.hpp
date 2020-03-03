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

FILE: Methane/Graphics/Noise.hpp
Multi-octave simplex noise generator in range [0, 1]

******************************************************************************/

#pragma once

#include "MathTypes.h"

#include <simplexnoise1234.h>

#include <array>

namespace Methane::Graphics
{

template <size_t octaves_count = 4>
class NoiseOctaves
{
public:
    NoiseOctaves(float persistence = 0.5f)
        : m_weights(GetWeights(persistence))
        , m_norm_multiplier(0.5f / GetWeightsSum(m_weights))
    { }
    
    float operator()(Vector2f pos) const
    {
        float noise = 0.0f;
        for (size_t i = 0; i < octaves_count; ++i)
        {
            noise += m_weights[i] * SimplexNoise1234::noise(pos[0], pos[1]);
            pos *= 2.f;
        }
        return noise * m_norm_multiplier + 0.5f;
    }

    float operator()(Vector3f pos) const
    {
        float noise = 0.0f;
        for (size_t i = 0; i < octaves_count; ++i)
        {
            noise += m_weights[i] * SimplexNoise1234::noise(pos[0], pos[1], pos[2]);
            pos *= 2.f;
        }
        return noise * m_norm_multiplier + 0.5f;
    }

    float operator()(Vector4f pos) const
    {
        float noise = 0.f;
        for (size_t i = 0; i < octaves_count; ++i)
        {
            noise += m_weights[i] * SimplexNoise1234::noise(pos[0], pos[1], pos[2], pos[3]);
            pos *= 2.f;
        }
        return noise * m_norm_multiplier + 0.5f;
    }

private:
    using WeightsArray = std::array<float, octaves_count>;
    
    static WeightsArray GetWeights(float persistence)
    {
        WeightsArray weights;
        for (size_t i = 0; i < octaves_count; ++i)
        {
            weights[i]   = persistence;
            persistence *= persistence;
        }
        return weights;
    }
    
    static float GetWeightsSum(const WeightsArray& weights)
    {
        float weights_sum = 0.f;
        for(float weight : weights)
        {
            weights_sum += weight;
        }
        return weights_sum;
    }
    
    const WeightsArray m_weights;
    const float        m_norm_multiplier;
};

}
