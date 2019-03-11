/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/ObjectBase.h
Base implementation of the named object interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Object.h>

namespace Methane
{
namespace Graphics
{

class ObjectBase : public virtual Object
{
public:
    ObjectBase() = default;
    virtual ~ObjectBase() override = default;

    // Object interface
    virtual void               SetName(const std::string& name) override { m_name = name; }
    virtual const std::string& GetName() const noexcept override         { return m_name; }

private:
    std::string m_name;
};

} // namespace Graphics
} // namespace Methane
