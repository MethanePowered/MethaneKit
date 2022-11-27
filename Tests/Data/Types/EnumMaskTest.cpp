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

FILE: Tests/Data/Types/EnumMastTest.cpp
Unit-tests of the EnumMask data type.

******************************************************************************/

#include <Methane/Data/EnumMask.hpp>
#include <Methane/Data/EnumMaskUtil.hpp>

#include <catch2/catch_template_test_macros.hpp>

using namespace Methane::Data;

template<typename E, typename M>
struct Catch::StringMaker<EnumMask<E, M>>
{
    static std::string convert(const EnumMask<E, M>& mask)
    {
        return GetEnumMaskName(mask);
    }
};

enum class Fruit : uint32_t
{
    Apple,
    Banana,
    Peach,
    Orange,
    Mandarin,
    Mango,
    Lime
};

template<typename MaskType, typename EnumType>
constexpr MaskType AsBit(EnumType e) noexcept
{
    return MaskType{ 1 } << static_cast<MaskType>(e);
}

template<typename MaskType, typename EnumType>
constexpr MaskType AsMask(std::initializer_list<EnumType> bits) noexcept
{
    MaskType mask{};
    for(EnumType bit : bits)
        mask |= AsBit<MaskType>(bit);
    return mask;
}

#define MASK_TYPES int32_t, uint32_t, int64_t, uint64_t

TEMPLATE_TEST_CASE("EnumMask Initialization", "[enum-mask][init]", MASK_TYPES)
{
    using EnumMaskType = EnumMask<Fruit, TestType>;

    SECTION("Default constructor")
    {
        constexpr EnumMaskType mask;
        CHECK(mask.ToInt() == TestType{});
    }

    SECTION("Mask value constructor")
    {
        constexpr EnumMaskType mask{ 3 };
        CHECK(mask.ToInt() == TestType{ 3 });
    }

    SECTION("Enum bit constructor")
    {
        constexpr EnumMaskType mask(Fruit::Mandarin);
        CHECK(mask.ToInt() == AsBit<TestType>(Fruit::Mandarin));
    }

    SECTION("Enum bits list constructor")
    {
        constexpr EnumMaskType mask({ Fruit::Apple, Fruit::Peach, Fruit::Mandarin });
        CHECK(mask.ToInt() == AsMask<TestType>({ Fruit::Apple, Fruit::Peach, Fruit::Mandarin }));
    }
}

TEMPLATE_TEST_CASE("EnumMask Comparison", "[enum-mask][compare]", MASK_TYPES)
{
    using EnumMaskType = EnumMask<Fruit, TestType>;
    constexpr EnumMaskType ref_mask{ 3 };

    SECTION("Equality operator")
    {
        CHECK(ref_mask == EnumMaskType{ 3 });
        CHECK_FALSE(ref_mask == EnumMaskType{ 4 });
    }

    SECTION("Inequality operator")
    {
        CHECK(ref_mask != EnumMaskType{ 4 });
        CHECK_FALSE(ref_mask != EnumMaskType{ 3 });
    }

    SECTION("Less operator")
    {
        CHECK(ref_mask < EnumMaskType{ 4 });
        CHECK_FALSE(ref_mask < EnumMaskType{ 3 });
        CHECK_FALSE(ref_mask < EnumMaskType{ 2 });
    }

    SECTION("Less or equal operator")
    {
        CHECK(ref_mask <= EnumMaskType{ 5 });
        CHECK(ref_mask <= EnumMaskType{ 3 });
        CHECK_FALSE(ref_mask <= EnumMaskType{ 2 });
    }

    SECTION("Greater operator")
    {
        CHECK(ref_mask > EnumMaskType{ 2 });
        CHECK_FALSE(ref_mask > EnumMaskType{ 3 });
        CHECK_FALSE(ref_mask > EnumMaskType{ 4 });
    }

    SECTION("Greater or equal operator")
    {
        CHECK(ref_mask >= EnumMaskType{ 2 });
        CHECK(ref_mask >= EnumMaskType{ 3 });
        CHECK_FALSE(ref_mask >= EnumMaskType{ 4 });
    }

    SECTION("Empty operator")
    {
        CHECK_FALSE(!ref_mask);
        CHECK(!EnumMaskType{});
    }
}

TEMPLATE_TEST_CASE("EnumMask bit operators", "[enum-mask][bit][operators]", MASK_TYPES)
{
    using EnumMaskType = EnumMask<Fruit, TestType>;
    constexpr EnumMaskType citrus_mask({ Fruit::Mandarin, Fruit::Lime, Fruit::Mango, Fruit::Orange });

    SECTION("Bit AND operator")
    {
        CHECK((citrus_mask & Fruit::Mandarin) == EnumMaskType({ Fruit::Mandarin }));
    }

    SECTION("Bit AND assignment operator")
    {
        EnumMaskType mask = citrus_mask;
        mask &= Fruit::Mandarin;
        CHECK(mask == EnumMaskType({ Fruit::Mandarin }));
    }

    SECTION("Bit OR operator")
    {
        CHECK((citrus_mask | Fruit::Apple) == EnumMaskType({ Fruit::Apple, Fruit::Mandarin, Fruit::Lime, Fruit::Mango, Fruit::Orange }));
    }

    SECTION("Bit OR assignment operator")
    {
        EnumMaskType mask = citrus_mask;
        mask |= Fruit::Apple;
        CHECK(mask == EnumMaskType({ Fruit::Apple, Fruit::Mandarin, Fruit::Lime, Fruit::Mango, Fruit::Orange }));
    }

    SECTION("Bit XOR operator")
    {
        CHECK((citrus_mask ^ Fruit::Mandarin) == EnumMaskType({ Fruit::Lime, Fruit::Mango, Fruit::Orange }));
    }

    SECTION("Bit XOR assignment operator")
    {
        EnumMaskType mask = citrus_mask;
        mask ^= Fruit::Mandarin;
        CHECK(mask == EnumMaskType({ Fruit::Lime, Fruit::Mango, Fruit::Orange }));
    }

    SECTION("Bit NOT operator")
    {
        constexpr EnumMaskType not_mask({ Fruit::Apple, Fruit::Peach, Fruit::Banana });
        CHECK((~citrus_mask).HasBits(not_mask));
        CHECK_FALSE(~citrus_mask == not_mask);
    }
}

TEMPLATE_TEST_CASE("EnumMask mask operators", "[enum-mask][operators]", MASK_TYPES)
{
    using EnumMaskType = EnumMask<Fruit, TestType>;
    constexpr EnumMaskType citrus_mask({ Fruit::Mandarin, Fruit::Lime, Fruit::Mango, Fruit::Orange });
    constexpr EnumMaskType misc_mask({ Fruit::Apple, Fruit::Peach, Fruit::Banana, Fruit::Mango, Fruit::Lime });

    SECTION("Mask AND operator")
    {
        CHECK((citrus_mask & misc_mask) == EnumMaskType({ Fruit::Mango, Fruit::Lime }));
    }

    SECTION("Bit AND assignment operator")
    {
        EnumMaskType mask = citrus_mask;
        mask &= misc_mask;
        CHECK(mask == EnumMaskType({ Fruit::Mango, Fruit::Lime }));
    }

    SECTION("Mask OR operator")
    {
        CHECK((citrus_mask | misc_mask) == EnumMaskType({ Fruit::Apple, Fruit::Banana, Fruit::Peach, Fruit::Orange, Fruit::Mandarin, Fruit::Mango, Fruit::Lime }));
    }

    SECTION("Mask OR assignment operator")
    {
        EnumMaskType mask = citrus_mask;
        mask |= misc_mask;
        CHECK(mask == EnumMaskType({ Fruit::Apple, Fruit::Banana, Fruit::Peach, Fruit::Orange, Fruit::Mandarin, Fruit::Mango, Fruit::Lime }));
    }

    SECTION("Mask XOR operator")
    {
        CHECK((citrus_mask ^ misc_mask) == EnumMaskType({ Fruit::Apple, Fruit::Banana, Fruit::Peach, Fruit::Orange, Fruit::Mandarin }));
    }

    SECTION("Mask XOR assignment operator")
    {
        EnumMaskType mask = citrus_mask;
        mask ^= misc_mask;
        CHECK(mask == EnumMaskType({ Fruit::Apple, Fruit::Banana, Fruit::Peach, Fruit::Orange, Fruit::Mandarin }));
    }
}

TEMPLATE_TEST_CASE("EnumMask conversion operators", "[enum-mask][convert]", MASK_TYPES)
{
    using EnumMaskType = EnumMask<Fruit, TestType>;
    constexpr EnumMaskType citrus_mask({ Fruit::Mandarin, Fruit::Lime, Fruit::Mango, Fruit::Orange });

    SECTION("Convert to boolean")
    {
        CHECK(static_cast<bool>(citrus_mask));
        CHECK_FALSE(static_cast<bool>(EnumMaskType{}));
    }

    SECTION("Convert to integer")
    {
        CHECK(static_cast<TestType>(citrus_mask) == AsMask<TestType>({ Fruit::Mandarin, Fruit::Lime, Fruit::Mango, Fruit::Orange }));
    }

    SECTION("Convert to vector of bits")
    {
        CHECK(GetEnumMaskBits(citrus_mask) == std::vector<Fruit>{ Fruit::Orange, Fruit::Mandarin, Fruit::Mango, Fruit::Lime });
    }

    SECTION("Convert to vector of bit names")
    {
        CHECK(GetEnumBitNames(citrus_mask) == std::vector<std::string>{ "Orange", "Mandarin", "Mango", "Lime" });
    }

    SECTION("Convert to string with default separator")
    {
        CHECK(GetEnumMaskName(citrus_mask) == "(Orange|Mandarin|Mango|Lime)");
    }

    SECTION("Convert to string with custom separator")
    {
        CHECK(GetEnumMaskName(citrus_mask, " + ") == "(Orange + Mandarin + Mango + Lime)");
    }
}

TEMPLATE_TEST_CASE("EnumMask bit operations", "[enum-mask][bit]", MASK_TYPES)
{
    using EnumMaskType = EnumMask<Fruit, TestType>;
    constexpr EnumMaskType citrus_mask({ Fruit::Mandarin, Fruit::Lime, Fruit::Mango, Fruit::Orange });

    SECTION("Check has bit")
    {
        CHECK(citrus_mask.HasBit(Fruit::Lime));
        CHECK_FALSE(citrus_mask.HasBit(Fruit::Banana));
    }

    SECTION("Check has any bit")
    {
        CHECK(citrus_mask.HasAnyBit(Fruit::Mango));
        CHECK_FALSE(citrus_mask.HasAnyBit(Fruit::Apple));
    }

    SECTION("Check has bits")
    {
        CHECK(citrus_mask.HasBits({ Fruit::Lime, Fruit::Orange }));
        CHECK_FALSE(citrus_mask.HasBits({ Fruit::Lime, Fruit::Banana }));
    }

    SECTION("Check has any bits")
    {
        CHECK(citrus_mask.HasAnyBits({ Fruit::Lime, Fruit::Banana }));
        CHECK_FALSE(citrus_mask.HasBits({ Fruit::Apple, Fruit::Banana }));
    }

    SECTION("Set bit on")
    {
        EnumMaskType mask = citrus_mask;
        CHECK_FALSE(mask.HasBit(Fruit::Banana));

        mask.SetBitOn(Fruit::Banana);

        CHECK(mask.HasBits({ Fruit::Banana, Fruit::Mandarin, Fruit::Lime, Fruit::Mango, Fruit::Orange }));
        CHECK_FALSE(mask.HasBits({ Fruit::Apple, Fruit::Peach }));
    }

    SECTION("Set bit off")
    {
        EnumMaskType mask = citrus_mask;
        CHECK(mask.HasBit(Fruit::Mango));

        mask.SetBitOff(Fruit::Mango);

        CHECK(mask.HasBits({ Fruit::Mandarin, Fruit::Lime, Fruit::Orange }));
        CHECK_FALSE(mask.HasBits({ Fruit::Apple, Fruit::Peach, Fruit::Banana, Fruit::Mango }));
    }

    SECTION("Set bit conditional")
    {
        EnumMaskType mask = citrus_mask;
        CHECK(mask.HasBit(Fruit::Orange));
        CHECK_FALSE(mask.HasBit(Fruit::Apple));

        mask.SetBit(Fruit::Orange, false);
        mask.SetBit(Fruit::Apple, true);

        CHECK(mask.HasBits({ Fruit::Apple, Fruit::Mandarin, Fruit::Lime, Fruit::Mango }));
        CHECK_FALSE(mask.HasBits({ Fruit::Peach, Fruit::Banana, Fruit::Orange }));
    }
}