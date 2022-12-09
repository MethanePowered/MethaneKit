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

//#define BENCHMARK_ENABLED

#ifdef BENCHMARK_ENABLED
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

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
    using EnumBitType = typename EnumMaskType::Bit;

    SECTION("Default constructor")
    {
        constexpr EnumMaskType mask;
        CHECK(mask.GetValue() == TestType{});
    }

    SECTION("Mask value constructor")
    {
        constexpr EnumMaskType mask{ 3 };
        CHECK(mask.GetValue() == TestType{ 3 });
    }

    SECTION("Enum bit constructor")
    {
        constexpr EnumMaskType mask(Fruit::Mandarin);
        CHECK(mask.GetValue() == AsBit<TestType>(Fruit::Mandarin));
    }

    SECTION("Enum indexed bit constructor")
    {
        constexpr EnumMaskType mask(EnumBitType(uint8_t(2)));
        CHECK(mask.GetValue() == AsBit<TestType>(Fruit::Peach));
    }

    SECTION("Enum bits list constructor")
    {
        constexpr EnumMaskType mask({ Fruit::Apple, Fruit::Peach, Fruit::Mandarin });
        CHECK(mask.GetValue() == AsMask<TestType>({ Fruit::Apple, Fruit::Peach, Fruit::Mandarin }));
    }

    SECTION("Mask copy constructor")
    {
        constexpr EnumMaskType orig_mask{ 3 };
        constexpr EnumMaskType copy_mask(orig_mask);
        CHECK(copy_mask.GetValue() == orig_mask.GetValue());
    }

    SECTION("Mask assignment operator")
    {
        constexpr EnumMaskType source_mask{ 5 };
        EnumMaskType target_mask{ 3 };
        target_mask = source_mask;
        CHECK(target_mask.GetValue() == source_mask.GetValue());
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

TEMPLATE_TEST_CASE("EnumMask Bit type", "[enum-mask][bit]", MASK_TYPES)
{
    using EnumMaskType = EnumMask<Fruit, TestType>;
    using EnumMaskBitType = typename EnumMaskType::Bit;

    SECTION("Bit constructor by index")
    {
        for(uint8_t i = 0; i < magic_enum::enum_count<Fruit>(); ++i)
        {
            const EnumMaskBitType bit(i);
            CHECK(bit.GetValue() == TestType(1) << i);
            CHECK(bit.GetIndex() == i);
            CHECK(bit.GetEnum() == magic_enum::enum_value<Fruit>(i));
        }
    }

    SECTION("Bit constructor by enum")
    {
        for(Fruit fruit : magic_enum::enum_values<Fruit>())
        {
            const EnumMaskBitType bit(fruit);
            CHECK(bit.GetValue() == TestType(1) << static_cast<TestType>(fruit));
            CHECK(bit.GetIndex() == static_cast<TestType>(fruit));
            CHECK(bit.GetEnum() == fruit);
        }
    }

    SECTION("Bit constexpr constructor for Apple")
    {
        constexpr EnumMaskBitType apple_bit(Fruit::Apple);
        constexpr bool is_apple = apple_bit.GetEnum() == Fruit::Apple;
        CHECK(is_apple);
    }

    SECTION("Bit constexpr constructor for Banana")
    {
        constexpr EnumMaskBitType banana_bit(Fruit::Banana);
        constexpr bool is_banana = banana_bit.GetEnum() == Fruit::Banana;
        CHECK(is_banana);
    }

    SECTION("Bit constexpr constructor for Apple")
    {
        constexpr EnumMaskBitType mango_bit(Fruit::Mango);
        constexpr bool is_mango = mango_bit.GetEnum() == Fruit::Mango;
        CHECK(is_mango);
    }

#ifdef BENCHMARK_ENABLED
    BENCHMARK("ForEachBitInEnumMask benchmark")
    {
        constexpr EnumMaskType citrus_mask({ Fruit::Mandarin, Fruit::Lime, Fruit::Mango, Fruit::Orange });
        size_t wrong_bits_count = 0;
        size_t bits_count = 0;
        ForEachBitInEnumMask(citrus_mask, [&bits_count, &wrong_bits_count](Fruit f)
        {
            bits_count++;
            if (f != Fruit::Mandarin && f != Fruit::Lime && f != Fruit::Mango && f != Fruit::Orange)
                wrong_bits_count++;
        });
        return bits_count + wrong_bits_count;
    };
#endif
}
