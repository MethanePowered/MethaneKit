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

******************************************************************************/

#pragma once

#include <stdexcept>
#include <string>

namespace Methane
{

// ========================= Argument exceptions =========================

class ArgumentException
{
public:
    ArgumentException(const std::string& function_name, const std::string& argument_name)
        : m_function_name(function_name)
        , m_argument_name(argument_name)
    { }

    const std::string& GetFunctionName() const noexcept { return m_function_name; }
    const std::string& GetArgumentName() const noexcept { return m_argument_name; }

private:
    const std::string m_function_name;
    const std::string m_argument_name;
};

template<typename BaseExceptionType>
class ArgumentExceptionBase
    : public BaseExceptionType
    , public ArgumentException
{
public:
    using ArgumentExceptionBaseType = ArgumentExceptionBase<BaseExceptionType>;

    ArgumentExceptionBase(const std::string& function_name, const std::string& argument_name, const std::string& invalid_msg, const std::string& description = "")
        : BaseExceptionType(std::string("Function \"") + function_name + "\" argument \"" + argument_name +
                            "\" value " + invalid_msg + (description.empty() ? "" : (": " + description)))
        , ArgumentException(function_name, argument_name)
    { }

protected:
    static std::string ToString(const std::string& value) { return value; }

    template<typename V, typename = std::enable_if_t<!std::is_arithmetic_v<V> && !std::is_same_v<V, std::string>, void>>
    static std::string ToString(const V& value) { return static_cast<std::string>(value); }

    template<typename V, typename = std::enable_if_t<std::is_arithmetic_v<V>, void>>
    static std::string ToString(V value) { return std::to_string(value); }
};

template<typename T>
class InvalidArgumentException : public ArgumentExceptionBase<std::invalid_argument>
{
public:
    InvalidArgumentException(const std::string& function_name, const std::string& argument_name, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, argument_name, "is not valid", description)
    { }

    InvalidArgumentException(const std::string& function_name, const std::string& argument_name, T value, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, argument_name, std::string(typeid(T).name()) + "(" + ToString(value) + ") is not valid", description)
        , m_value(std::move(value))
    { }

    const T& GetValue() const noexcept { return m_value; }

private:
    const T m_value{ };
};

template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>, void>>
class OutOfRangeArgumentException : public ArgumentExceptionBase<std::out_of_range>
{
public:
    OutOfRangeArgumentException(const std::string& function_name, const std::string& argument_name, T value, std::pair<T, T> range, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, argument_name, std::string(typeid(T).name()) + "(" + ToString(value) +
                                    ") is out of range [" + std::to_string(range.first) + ", " + std::to_string(range.second) + ")", description)
        , m_value(std::move(value))
        , m_range(std::move(range))
    { }

    const T& GetValue() const noexcept               { return m_value; }
    const std::pair<T, T>& GetRange() const noexcept { return m_range; }

private:
    const T m_value;
    const std::pair<T, T> m_range;
};

template<typename T>
class EmptyArgumentException : public ArgumentExceptionBase<std::invalid_argument>
{
public:
    EmptyArgumentException(const std::string& function_name, const std::string& argument_name, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, argument_name, std::string("is empty container of type ") + typeid(T).name(), description)
    { }
};

template<typename T>
class NullPointerArgumentException : public ArgumentExceptionBase<std::invalid_argument>
{
public:
    NullPointerArgumentException(const std::string& function_name, const std::string& argument_name, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, argument_name, std::string("is null pointer of type ") + typeid(T).name(), description)
    { }
};

template<typename T>
class ZeroArgumentException : public ArgumentExceptionBase<std::invalid_argument>
{
public:
    ZeroArgumentException(const std::string& function_name, const std::string& argument_name, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, argument_name, std::string("is zero value of type ") + typeid(T).name(), description)
    { }
};

template<typename T, typename = std::enable_if_t<std::is_enum_v<T>, void>>
class UnexpectedEnumArgumentException : public ArgumentExceptionBase<std::invalid_argument>
{
public:
    UnexpectedEnumArgumentException(const std::string& function_name, const std::string& variable_name, T value, const std::string& description = "")
        : ArgumentExceptionBaseType(function_name, variable_name, std::string("enum value ") + typeid(T).name() + "(" + std::to_string(static_cast<uint32_t>(value)) + ") is unexpected", description)
        , m_value(value)
    { }

    T GetValue() const noexcept { m_value; }

private:
    const T m_value;
};

} // namespace Methane
