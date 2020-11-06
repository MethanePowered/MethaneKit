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

#pragma once

#include "Item.h"

namespace Methane::UserInterface
{

class Container;

struct IContainerCallback
{
    virtual void ChildrenChanged(Container& container) = 0;

    virtual ~IContainerCallback() = default;
};

class Container
    : public Item
    , public Data::Emitter<IContainerCallback>
{
public:
    Container(Context& ui_context, const UnitRect& ui_rect = {}, const Ptrs<Item>& children = {});

    const Ptrs<Item>& GetChildren() const noexcept { return m_children; }

    // Container interface
    virtual bool AddChild(Item& item);
    virtual bool RemoveChild(Item& item);

    // Item overrides
    bool SetRect(const UnitRect& ui_rect) override;

private:
    Ptrs<Item> m_children;
};

} // namespace Methane::UserInterface
