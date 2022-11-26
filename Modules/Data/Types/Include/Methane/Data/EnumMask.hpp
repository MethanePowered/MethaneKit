/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Data/EnumMask.hpp
Enum-based mask data type with common mask operations.

******************************************************************************/

#pragma once

#include <cstdint>
#include <initializer_list>

namespace Methane::Data
{

template<typename E, typename M = std::underlying_type_t<E>>
class EnumMask
{
    static_assert(std::is_enum_v<E>, "EnumMask enum-type has to be enum type.");
    static_assert(std::is_integral_v<M>, "EnumMask mask-type has to be integer type.");
    static_assert(sizeof(E) <= sizeof(M), "EnumMask storage type size is less than enum-type size which results in overflow.");

public:
    using EnumType = E;
    using MaskType = M;

    constexpr static M AsInt(E e) noexcept { return M{ 1 } << static_cast<M>(e); }

    constexpr EnumMask() noexcept = default;
    constexpr explicit EnumMask(M value) noexcept : m_value(value) { }
    constexpr explicit EnumMask(E bit) noexcept : m_value(AsInt(bit)) { }
    constexpr EnumMask(std::initializer_list<E> bits) : m_value(AsInt(bits.begin(), bits.end())) { }

    constexpr M ToInt() const noexcept { return m_value; }

    constexpr bool operator==(const EnumMask& other) const noexcept { return m_value == other.m_value; }
    constexpr bool operator!=(const EnumMask& other) const noexcept { return m_value != other.m_value; }
    constexpr bool operator<(const EnumMask& other) const noexcept  { return m_value < other.m_value;  }
    constexpr bool operator<=(const EnumMask& other) const noexcept { return m_value <= other.m_value; }
    constexpr bool operator>(const EnumMask& other) const noexcept  { return m_value > other.m_value;  }
    constexpr bool operator>=(const EnumMask& other) const noexcept { return m_value >= other.m_value; }
    constexpr bool operator!() const noexcept                       { return !m_value; }

    constexpr EnumMask operator|(E bit) const noexcept          { return EnumMask(m_value | AsInt(bit)); }
    constexpr EnumMask operator|(EnumMask mask) const noexcept  { return EnumMask(m_value | mask.ToInt()); }
    constexpr EnumMask operator&(E bit) const noexcept          { return EnumMask(m_value & AsInt(bit)); }
    constexpr EnumMask operator&(EnumMask mask) const noexcept  { return EnumMask(m_value & mask.ToInt()); }
    constexpr EnumMask operator^(E bit) const noexcept          { return EnumMask(m_value ^ AsInt(bit)); }
    constexpr EnumMask operator^(EnumMask mask) const noexcept  { return EnumMask(m_value ^ mask.ToInt()); }
    constexpr EnumMask operator~() const noexcept               { return EnumMask(~m_value); }

    constexpr EnumMask& operator|=(E bit) noexcept          { m_value |= AsInt(bit); return *this; }
    constexpr EnumMask& operator|=(EnumMask mask) noexcept  { m_value |= mask.ToInt(); return *this; }
    constexpr EnumMask& operator&=(E bit) noexcept          { m_value &= AsInt(bit); return *this; }
    constexpr EnumMask& operator&=(EnumMask mask) noexcept  { m_value &= mask.ToInt(); return *this; }
    constexpr EnumMask& operator^=(E bit) noexcept          { m_value ^= AsInt(bit); return *this; }
    constexpr EnumMask& operator^=(EnumMask mask) noexcept  { m_value ^= mask.ToInt(); return *this; }

    constexpr operator bool() const noexcept { return m_value != M{ 0 }; }
    constexpr operator M() const noexcept    { return m_value; }

    constexpr EnumMask& SetBitOn(E bit) noexcept            { return *this |= bit; }
    constexpr EnumMask& SetBitOff(E bit) noexcept           { return *this &= ~EnumMask(bit); }
    constexpr EnumMask& SetBit(E bit, bool on) noexcept     { return on ? SetBitOn(bit) : SetBitOff(bit); }
    constexpr bool HasBits(EnumMask mask) const noexcept    { return mask.m_value ? ((m_value & mask.m_value) == mask.m_value) : !m_value; }
    constexpr bool HasBit(E bit) const noexcept             { return HasBits(EnumMask(bit)); }
    constexpr bool HasAnyBits(EnumMask mask) const noexcept { return (m_value & mask.m_value) != M{ 0 }; }
    constexpr bool HasAnyBit(E bit) const noexcept          { return HasAnyBits(EnumMask(bit)); }

private:
    constexpr static M AsInt(typename std::initializer_list<E>::const_iterator it,
                                    typename std::initializer_list<E>::const_iterator end) noexcept
    {
        return it == end ? M{ 0 } : AsInt(*it) | AsInt(it + 1, end);
    }

    M m_value{ 0 };
};

} // namespace Methane::Data
