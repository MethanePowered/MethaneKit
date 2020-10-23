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

FILE: Methane/UserInterface/Container.h
Methane user interface container of items.

******************************************************************************/

#include <Methane/UserInterface/Container.h>

#include <Methane/Instrumentation.h>

namespace Methane::UserInterface
{

Container::Container(Context& ui_context, const UnitRect& ui_rect, Ptrs<Item> children)
    : Item(ui_context, ui_rect)
    , m_children(children)
{
    META_FUNCTION_TASK();
}

bool Container::AddChild(Item& item)
{
    META_FUNCTION_TASK();
    const auto child_it = std::find_if(m_children.begin(), m_children.end(),
        [&item](const Ptr<Item>& item_ptr) { return item_ptr.get() == std::addressof(item); }
    );
    if (child_it != m_children.end())
        return false;

    m_children.emplace_back(item.GetPtr());
    Emitter<IContainerCallback>::Emit(&IContainerCallback::ChildrenChanged, *this);
    return true;
}

bool Container::RemoveChild(Item& item)
{
    META_FUNCTION_TASK();
    const auto child_it = std::find_if(m_children.begin(), m_children.end(),
        [&item](const Ptr<Item>& item_ptr) { return item_ptr.get() == std::addressof(item); }
    );
    if (child_it == m_children.end())
        return false;

    m_children.erase(child_it);
    Emitter<IContainerCallback>::Emit(&IContainerCallback::ChildrenChanged, *this);
    return true;
}

bool Container::SetRect(const UnitRect& ui_rect)
{
    META_FUNCTION_TASK();
    if (!Item::SetRect(ui_rect))
        return false;

    const UnitPoint& panel_origin_px = GetRectInPixels().GetUnitOrigin();
    for (const Ptr<Item>& child_item_ptr : GetChildren())
    {
        assert(child_item_ptr);
        child_item_ptr->SetOrigin(panel_origin_px + child_item_ptr->GetRelOriginInPixels());
    }

    return true;
}

} // namespace Methane::UserInterface
