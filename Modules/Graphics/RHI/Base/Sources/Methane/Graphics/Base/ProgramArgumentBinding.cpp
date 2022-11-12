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

FILE: Methane/Graphics/Base/ProgramArgumentBinding.cpp
Base implementation of the program argument binding interface.

******************************************************************************/

#include <Methane/Graphics/Base/ProgramArgumentBinding.h>
#include <Methane/Graphics/Base/ProgramBindings.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

template<>
struct fmt::formatter<Methane::Graphics::IResource::View>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::IResource::View& rl, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(rl)); }
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<>
struct fmt::formatter<Methane::Graphics::ProgramArgumentAccessor>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::ProgramArgumentAccessor& rl, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(rl)); }
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

namespace Methane::Graphics::Base
{

ProgramArgumentBinding::ProgramArgumentBinding(const Context& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
{
    META_FUNCTION_TASK();
}

void ProgramArgumentBinding::MergeSettings(const ProgramArgumentBinding& other)
{
    META_FUNCTION_TASK();
    const Settings& settings = other.GetSettings();
    META_CHECK_ARG_EQUAL(settings.argument, m_settings.argument);
    META_CHECK_ARG_EQUAL(settings.resource_type, m_settings.resource_type);
    META_CHECK_ARG_EQUAL(settings.resource_count, m_settings.resource_count);
}

bool ProgramArgumentBinding::SetResourceViews(const IResource::Views& resource_views)
{
    META_FUNCTION_TASK();
    if (m_resource_views == resource_views)
        return false;

    if (m_settings.argument.IsConstant() && !m_resource_views.empty())
        throw ConstantModificationException(GetSettings().argument);

    META_CHECK_ARG_NOT_EMPTY_DESCR(resource_views, "can not set empty resources for resource binding");

    const bool            is_addressable_binding = m_settings.argument.IsAddressable();
    const IResource::Type bound_resource_type    = m_settings.resource_type;
    META_UNUSED(is_addressable_binding);
    META_UNUSED(bound_resource_type);

    for (const IResource::View& resource_view : resource_views)
    {
        META_CHECK_ARG_NAME_DESCR("resource_view", resource_view.GetResource().GetResourceType() == bound_resource_type,
                                  "incompatible resource type '{}' is bound to argument '{}' of type '{}'",
                                  magic_enum::enum_name(resource_view.GetResource().GetResourceType()),
                                  m_settings.argument.GetName(), magic_enum::enum_name(bound_resource_type));

        const IResource::Usage resource_usage_mask = resource_view.GetResource().GetUsage();
        using namespace magic_enum::bitwise_operators;
        META_CHECK_ARG_DESCR(resource_usage_mask, static_cast<bool>(resource_usage_mask & IResource::Usage::Addressable) == is_addressable_binding,
                             "resource addressable usage flag does not match with resource binding state");
        META_CHECK_ARG_NAME_DESCR("resource_view", is_addressable_binding || !resource_view.GetOffset(),
                                  "can not set resource view_id with non-zero offset to non-addressable resource binding");
    }

    Data::Emitter<IProgramBindings::IArgumentBindingCallback>::Emit(&IProgramBindings::IArgumentBindingCallback::OnProgramArgumentBindingResourceViewsChanged, std::cref(*this), std::cref(m_resource_views), std::cref(resource_views));

    m_resource_views = resource_views;
    return true;
}

ProgramArgumentBinding::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("{} is bound to {}", m_settings.argument, fmt::join(m_resource_views, ", "));
}

bool ProgramArgumentBinding::IsAlreadyApplied(const IProgram& program,
                                                  const ProgramBindings& applied_program_bindings,
                                                  bool check_binding_value_changes) const
{
    META_FUNCTION_TASK();
    if (std::addressof(applied_program_bindings.GetProgram()) != std::addressof(program))
        return false;

    // 1) No need in setting constant resource binding
    //    when another binding was previously set in the same command list for the same program
    if (m_settings.argument.IsConstant())
        return true;

    if (!check_binding_value_changes)
        return false;

    // 2) No need in setting resource binding to the same location
    //    as a previous resource binding set in the same command list for the same program
    if (const IProgramBindings::IArgumentBinding& previous_argument_argument_binding = applied_program_bindings.Get(m_settings.argument);
        previous_argument_argument_binding.GetResourceViews() == m_resource_views)
        return true;

    return false;
}

} // namespace Methane::Graphics::Base
