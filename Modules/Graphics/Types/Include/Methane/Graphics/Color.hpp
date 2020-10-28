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

FILE: Methane/Graphics/Color.hpp
Color type based on cml::vector.

******************************************************************************/

#pragma once

#include "Types.h"

#include <cstdint>

namespace Methane::Graphics
{

class Color3f : public Vector3f
{
public:
    using Vector3f::Vector3f;
    using Vector3f::operator=;

    Color3f() = default;
    Color3f(float r, float g, float b) : Vector3f(r, g, b) { }

    float GetR() const noexcept { return (*this)[0]; }
    float GetG() const noexcept { return (*this)[1]; }
    float GetB() const noexcept { return (*this)[2]; }

    void SetR(float r) noexcept { (*this)[0] = r; }
    void SetG(float g) noexcept { (*this)[1] = g; }
    void SetB(float b) noexcept { (*this)[2] = b; }

    std::string ToString() const
    {
        return "C(R:" + std::to_string(GetR()) +
               ", G:" + std::to_string(GetG()) +
               ", B:" + std::to_string(GetB()) + ")";
    }
};

class Color4f : public Vector4f
{
public:
    using Vector4f::Vector4f;
    using Vector4f::operator=;

    Color4f() : Vector4f(0.F, 0.F, 0.F, 0.F) { }
    Color4f(float r, float g, float b, float a) : Vector4f(r, g, b, a) { }
    Color4f(Color3f c, float a) : Vector4f(std::move(c), a) { }

    float GetR() const noexcept { return (*this)[0]; }
    float GetG() const noexcept { return (*this)[1]; }
    float GetB() const noexcept { return (*this)[2]; }
    float GetA() const noexcept { return (*this)[3]; }

    void SetR(float r) noexcept { (*this)[0] = r; }
    void SetG(float g) noexcept { (*this)[1] = g; }
    void SetB(float b) noexcept { (*this)[2] = b; }
    void SetA(float a) noexcept { (*this)[3] = a; }

    std::string ToString() const
    {
        return "C(R:" + std::to_string(GetR()) +
               ", G:" + std::to_string(GetG()) +
               ", B:" + std::to_string(GetB()) +
               ", A:" + std::to_string(GetA()) + ")";
    }
};

} // namespace Methane::Graphics
