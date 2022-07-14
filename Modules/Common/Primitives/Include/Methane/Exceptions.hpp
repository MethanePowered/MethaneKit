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

FILE: Methane/Exceptions.h
Methane common exception types

 - InvalidArgumentException<T>
 - OutOfRangeArgumentException<T>
 - EmptyArgumentException<T>
 - NullPointerArgumentException<T>
 - ZeroArgumentException<T>
 - UnexpectedEnumArgumentException<T>
 - NotImplementedException

******************************************************************************/
#pragma once

#include <fmt/format.h>
#include <magic_enum.hpp>

#include <stdexcept>
#include <string>

namespace Methane
{

template <typename T, typename C, typename = C>
struct IsStaticCastable : std::false_type { };

template <typename T, typename C>
struct IsStaticCastable<T, C, decltype(static_cast<C>(std::declval<T>()))> : std::true_type { };

// ========================= Argument exceptions =========================

class ArgumentException
{
public:
    ArgumentException(const std::string& function_name, const std::string& argument_name)
        : m_function_name(function_name)
        , m_argument_name(argument_name)
    { }

    [[nodiscard]] const std::string& GetFunctionName() const noexcept { return m_function_name; }
    [[nodiscard]] const std::string& GetArgumentName() const noexcept { return m_argument_name; }

private:
    std::string m_function_name;
    std::string m_argument_name;
};

template<typename BaseExceptionType>
class ArgumentExceptionBase
    : public BaseExceptionType
    , public ArgumentException
{
public:
    using ArgumentExceptionBaseType = ArgumentExceptionBase<BaseExceptionType>;

    ArgumentExceptionBase(const std::string& function_name, const std::string& argument_name, const std::string& invalid_msg, const std::string& description = "")
        : BaseExceptionType(fmt::format("Function '{}' argument '{}' value {}.", function_name, argument_name, invalid_msg + (description.empty() ? "" : (": " + description))))
        , ArgumentException(function_name, argument_name)
    { }
};

template<typename T>
class InvalidArgumentException : public ArgumentExceptionBase<std::invalid_argument>
{
public:
    using DecayType = typename std::decay_t<T>;

    InvalidArgumentException(const std::string& function_name, const std::string& argument_name, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, argument_name, "is not valid", description)
    { }

    InvalidArgumentException(const std::string& function_name, const std::string& argument_name, DecayType value, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, argument_name, GetMessage(value), description)
        , m_value(std::move(value))
    { }

    [[nodiscard]] const DecayType& GetValue() const noexcept { return m_value; }

private:
    template<typename V = DecayType>
    [[nodiscard]] static std::string GetMessage(const V& value) noexcept
    {
        if constexpr (std::is_enum_v<V>)
            return fmt::format("{}::{}({}) is not valid", magic_enum::enum_type_name<V>(), magic_enum::enum_name(value), magic_enum::enum_integer(value));
        else if constexpr (std::is_pointer_v<V>)
            return fmt::format("{}*({}) is not valid", typeid(V).name(), fmt::ptr(value));
        else if constexpr (IsStaticCastable<V, std::string>::value)
            return fmt::format("{}({}) is not valid", typeid(V).name(), static_cast<std::string>(value));
        else
            return fmt::format("{}({}) is not valid", typeid(V).name(), value);
    }

    DecayType m_value{ };
};

template<typename T, typename V, typename RangeType = std::pair<typename std::decay_t<V>, typename std::decay_t<V>>>
class OutOfRangeArgumentException : public ArgumentExceptionBase<std::out_of_range>
{
public:
    using DecayType = typename std::decay_t<T>;

    OutOfRangeArgumentException(const std::string& function_name, const std::string& argument_name, DecayType value, RangeType range, bool range_end_inclusive = false, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, argument_name, fmt::format("{}({}) is out of range [{}, {}{}", typeid(T).name(), value, range.first, range.second, range_end_inclusive ? ']' : ')'), description)
        , m_value(std::move(value))
        , m_range(std::move(range))
    { }

    [[nodiscard]] const T& GetValue() const noexcept               { return m_value; }
    [[nodiscard]] const std::pair<T, T>& GetRange() const noexcept { return m_range; }

private:
    DecayType m_value;
    RangeType m_range;
};

template<typename T>
class EmptyArgumentException : public ArgumentExceptionBase<std::invalid_argument>
{
public:
    EmptyArgumentException(const std::string& function_name, const std::string& argument_name, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, argument_name, fmt::format("is an empty container {}", typeid(T).name()), description)
    { }
};

template<typename T>
class NullPointerArgumentException : public ArgumentExceptionBase<std::invalid_argument>
{
public:
    NullPointerArgumentException(const std::string& function_name, const std::string& argument_name, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, argument_name, fmt::format("is null pointer of type {}", typeid(T).name()), description)
    { }
};

template<typename T>
class ZeroArgumentException : public ArgumentExceptionBase<std::invalid_argument>
{
public:
    ZeroArgumentException(const std::string& function_name, const std::string& argument_name, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, argument_name, fmt::format("is zero of type {}", typeid(T).name()), description)
    { }
};

template<typename T>
class UnexpectedArgumentException : public ArgumentExceptionBase<std::invalid_argument>
{
public:
    using DecayType = typename std::decay_t<T>;

    UnexpectedArgumentException(const std::string& function_name, const std::string& variable_name, DecayType value, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, variable_name, GetMessage(value), description)
        , m_value(std::move(value))
    { }

    [[nodiscard]] const DecayType& GetValue() const noexcept { m_value; }

private:
    template<typename V = DecayType>
    static std::string GetMessage(const V& value)
    {
        if constexpr (std::is_enum_v<V>)
            return fmt::format("{}::{}({}) is unexpected", magic_enum::enum_type_name<V>(), magic_enum::enum_name(value), magic_enum::enum_integer(value));
        else
            return fmt::format("{}({}) is unexpected", typeid(V).name(), value);
    }

    DecayType m_value;
};

class NotImplementedException : public std::logic_error
{
public:
    NotImplementedException(const std::string& function_name, const std::string& description = "")
        : std::logic_error(fmt::format("Function '{}' is not implemented{}.", function_name, description.empty() ? "" : fmt::format(": {}", description)))
        , m_function_name(function_name)
    { }

    [[nodiscard]] const std::string& GetFunctionName() const noexcept { return m_function_name; }

private:
    std::string m_function_name;
};

} // namespace Methane
