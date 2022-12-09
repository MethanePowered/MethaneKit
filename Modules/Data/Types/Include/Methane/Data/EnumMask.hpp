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
#include <cmath>
#include <initializer_list>
#include <type_traits>

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

    class Bit
    {
    public:
        explicit constexpr Bit(M i) noexcept : m_value(M{ 1 } << i) { }
        constexpr Bit(E e) noexcept : Bit(static_cast<M>(e)) { }

        constexpr M GetValue() const noexcept { return m_value; }
        constexpr operator M() const noexcept { return m_value; }

        constexpr M GetIndex() const noexcept { return floorLog2(m_value); }
        constexpr E GetEnum() const noexcept  { return static_cast<E>(GetIndex()); }

    private:
        static constexpr M floorLog2(M x)
        {
            return x == 1 ? 0 : 1 + floorLog2(x >> 1);
        }

        const M m_value;
    };

    constexpr EnumMask() noexcept = default;
    constexpr explicit EnumMask(M value) noexcept : m_value(value) { }
    constexpr explicit EnumMask(Bit bit) noexcept : m_value(bit.GetValue()) { }
    constexpr EnumMask(std::initializer_list<Bit> bits) noexcept : m_value(BitsToInt(bits.begin(), bits.end())) { }

    constexpr M GetValue() const noexcept { return m_value; }

    constexpr bool operator==(const EnumMask& other) const noexcept { return m_value == other.m_value; }
    constexpr bool operator!=(const EnumMask& other) const noexcept { return m_value != other.m_value; }
    constexpr bool operator<(const EnumMask& other) const noexcept  { return m_value <  other.m_value; }
    constexpr bool operator<=(const EnumMask& other) const noexcept { return m_value <= other.m_value; }
    constexpr bool operator>(const EnumMask& other) const noexcept  { return m_value >  other.m_value; }
    constexpr bool operator>=(const EnumMask& other) const noexcept { return m_value >= other.m_value; }
    constexpr bool operator!() const noexcept                       { return !m_value; }

    constexpr EnumMask operator|(Bit bit) const noexcept        { return EnumMask(m_value | bit.GetValue());  }
    constexpr EnumMask operator|(EnumMask mask) const noexcept  { return EnumMask(m_value | mask.GetValue()); }
    constexpr EnumMask operator&(Bit bit) const noexcept        { return EnumMask(m_value & bit.GetValue());  }
    constexpr EnumMask operator&(EnumMask mask) const noexcept  { return EnumMask(m_value & mask.GetValue()); }
    constexpr EnumMask operator^(Bit bit) const noexcept        { return EnumMask(m_value ^ bit.GetValue());  }
    constexpr EnumMask operator^(EnumMask mask) const noexcept  { return EnumMask(m_value ^ mask.GetValue()); }
    constexpr EnumMask operator~() const noexcept               { return EnumMask(~m_value); }

    constexpr EnumMask& operator|=(Bit bit) noexcept        { m_value |= bit.GetValue();  return *this; }
    constexpr EnumMask& operator|=(EnumMask mask) noexcept  { m_value |= mask.GetValue(); return *this; }
    constexpr EnumMask& operator&=(Bit bit) noexcept        { m_value &= bit.GetValue();  return *this; }
    constexpr EnumMask& operator&=(EnumMask mask) noexcept  { m_value &= mask.GetValue(); return *this; }
    constexpr EnumMask& operator^=(Bit bit) noexcept        { m_value ^= bit.GetValue();  return *this; }
    constexpr EnumMask& operator^=(EnumMask mask) noexcept  { m_value ^= mask.GetValue(); return *this; }

    constexpr operator bool() const noexcept { return m_value != M{}; }
    constexpr operator M() const noexcept    { return m_value; }

    constexpr EnumMask& SetBitOn(Bit bit) noexcept          { return *this |= bit; }
    constexpr EnumMask& SetBitOff(Bit bit) noexcept         { return *this &= ~EnumMask(bit); }
    constexpr EnumMask& SetBit(Bit bit, bool on) noexcept   { return on ? SetBitOn(bit) : SetBitOff(bit); }
    constexpr bool HasBits(EnumMask mask) const noexcept    { return mask.m_value ? ((m_value & mask.GetValue()) == mask.GetValue()) : !m_value; }
    constexpr bool HasBit(Bit bit) const noexcept           { return HasBits(EnumMask(bit)); }
    constexpr bool HasAnyBits(EnumMask mask) const noexcept { return (m_value & mask.GetValue()) != M{}; }
    constexpr bool HasAnyBit(Bit bit) const noexcept        { return HasAnyBits(EnumMask(bit)); }

private:
    constexpr static M BitsToInt(typename std::initializer_list<Bit>::const_iterator it,
                                 typename std::initializer_list<Bit>::const_iterator end) noexcept
    {
        return it == end ? M{ 0 } : it->GetValue() | BitsToInt(it + 1, end);
    }

    M m_value{ };
};

} // namespace Methane::Data
