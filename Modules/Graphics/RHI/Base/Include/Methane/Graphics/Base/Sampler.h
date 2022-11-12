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

FILE: Methane/Graphics/Base/Sampler.h
Base implementation of the sampler interface.

******************************************************************************/

#pragma once

#include "Resource.h"

#include <Methane/Graphics/ISampler.h>

namespace Methane::Graphics::Base
{

class Context;

class Sampler
    : public ISampler
    , public Resource
{
public:
    Sampler(const Context& context, const Settings& settings,
                State initial_state = State::Undefined, Opt<State> auto_transition_source_state_opt = {});

    // ISampler interface
    const Settings& GetSettings() const override { return m_settings; }

    // IResource interface
    void        SetData(const SubResources& sub_resources, ICommandQueue&) override;
    Data::Size  GetDataSize(Data::MemoryState) const noexcept override { return 0; }

private:
    Settings m_settings;
};

} // namespace Methane::Graphics::Base
