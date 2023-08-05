/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/HlslCatchHelpers.hpp
Catch2 unit-test helpers for HLSL++ vector types

******************************************************************************/

#include <Methane/Data/TypeFormatters.hpp>

#include <hlsl++.h>
#include <catch2/matchers/catch_matchers.hpp>

namespace Catch
{

template<typename HlslVectorType>
struct StringMaker<HlslVectorType>
{
    static std::string convert(const HlslVectorType& v) { return fmt::format("{}", v); }
};

} // namespace Catch

namespace Methane
{

template<typename HlslVectorType>
class HlslVectorEqualsMatcher
    : public Catch::Matchers::MatcherBase<HlslVectorType>
{
public:
    HlslVectorEqualsMatcher(const HlslVectorType& reference_vector)
        : m_reference_vector(reference_vector)
    { }

    bool match(const HlslVectorType& other) const override
    {
        return hlslpp::all(other == m_reference_vector);
    }

    std::string describe() const override
    {
        return fmt::format("equals to {}", m_reference_vector);
    }

private:
    HlslVectorType m_reference_vector;
};

template<typename HlslVectorType>
auto HlslVectorEquals(const HlslVectorType& reference_vector) -> decltype(auto)
{
    return HlslVectorEqualsMatcher<HlslVectorType>(reference_vector);
}

template<typename HlslVectorType>
class HlslVectorApproxEqualsMatcher
    : public Catch::Matchers::MatcherBase<HlslVectorType>
{
public:
    template<typename ComponentType>
    HlslVectorApproxEqualsMatcher(const HlslVectorType& reference_vector, ComponentType precision)
        : m_reference_vector(reference_vector)
        , m_precision_vector(precision)
    { }

    bool match(const HlslVectorType& other) const override
    {
        return hlslpp::all(hlslpp::abs(other - m_reference_vector) <= m_precision_vector);
    }

    std::string describe() const override
    {
        return fmt::format("approximately equals to {} with precision {}", m_reference_vector, m_precision_vector[0]);
    }

private:
    HlslVectorType m_reference_vector;
    HlslVectorType m_precision_vector;
};

template<typename HlslVectorType, typename ComponentType>
auto HlslVectorApproxEquals(const HlslVectorType& reference_vector, ComponentType precision) -> decltype(auto)
{
    return HlslVectorApproxEqualsMatcher<HlslVectorType>(reference_vector, precision);
}

} // namespace Methane
