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

FILE: Methane/Graphics/Metal/ResourceMT.h
Metal implementation of the resource interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ResourceBase.h>

#include <memory>

namespace Methane
{
namespace Graphics
{

class ContextMT;
struct ResourceContainerMT;

class ResourceMT : public ResourceBase
{
public:
    using Ptr = std::shared_ptr<ResourceMT>;

    class ReleasePoolMT : public ReleasePool
    {
    public:
        ReleasePoolMT();
        virtual ~ReleasePoolMT() override = default;

        // ReleasePool interface
        virtual void AddResource(ResourceBase& resource) override;
        virtual void ReleaseResources() override;

    private:
        std::unique_ptr<ResourceContainerMT> m_sp_mtl_resources;
    };

    ResourceMT(Type type, Usage::Mask usage_mask, ContextBase& context, const DescriptorByUsage& descriptor_by_usage);
    virtual ~ResourceMT() override = default;

protected:
    ContextMT& GetContextMT() noexcept;
};

} // namespace Graphics
} // namespace Methane
