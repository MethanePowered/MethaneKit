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

FILE: Methane/UserInterface/Item.h
Methane user interface item - base type of all user interface widgets and text.

******************************************************************************/

#pragma once

#include <Methane/UserInterface/Types.hpp>
#include <Methane/Data/Emitter.hpp>
#include <Methane/Memory.hpp>

namespace Methane::UserInterface
{

class Item;
class Context;

namespace gfx = Methane::Graphics;

struct IItemCallback
{
    virtual void RectChanged(Item& item) = 0;

    virtual ~IItemCallback() = default;
};

class Item
    : public Data::Emitter<IItemCallback>
    , public std::enable_shared_from_this<Item>
{
public:
    Item(Context& ui_context, const UnitRect& ui_rect = {});

    Ptr<Item>        GetPtr()                                           { return shared_from_this(); }
    const Context&   GetUIContext() const noexcept                      { return m_ui_context; }
    Context&         GetUIContext() noexcept                            { return m_ui_context; }
    const UnitPoint& GetRelOriginInPixels() const noexcept              { return m_rel_origin_px; }
    UnitPoint        GetRelOriginInDots() const noexcept;
    UnitPoint        GetRelOriginInUnits(Units units) const noexcept;
    const UnitRect&  GetRectInPixels() const noexcept                   { return m_abs_rect_px; }
    UnitRect         GetRectInDots() const noexcept;
    UnitRect         GetRectInUnits(Units units) const noexcept;

    virtual bool SetRect(const UnitRect& rect);

    void SetRelOrigin(const UnitPoint& rel_origin);
    bool SetOrigin(const UnitPoint& origin);
    bool SetSize(const UnitSize& size);

private:
    Context&  m_ui_context;
    UnitPoint m_rel_origin_px;
    UnitRect  m_abs_rect_px;
};

} // namespace Methane::UserInterface
