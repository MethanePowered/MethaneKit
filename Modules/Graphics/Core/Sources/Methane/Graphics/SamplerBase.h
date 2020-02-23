/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/SamplerBase.h
Base implementation of the sampler interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Sampler.h>

#include "Native/ResourceNT.h"

namespace Methane::Graphics
{

class ContextBase;

class SamplerBase
    : public Sampler
    , public ResourceNT
{
public:
    SamplerBase(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage);

    // Sampler interface
    const Settings& GetSettings() const override { return m_settings; }

    // Resource interface
    void        SetData(const SubResources& sub_resources) override;
    Data::Size  GetDataSize() const override { return 0; }

protected:
    ContextBase& GetContext() { return m_context; }

private:
    ContextBase& m_context;
    Settings     m_settings;
};

} // namespace Methane::Graphics
