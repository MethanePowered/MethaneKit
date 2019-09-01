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

FILE: Methane/Graphics/Metal/ShaderMT.hh
Metal implementation of the shader interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ShaderBase.h>

#import <Metal/Metal.h>

#include <string>
#include <memory>

namespace Methane
{
namespace Graphics
{

class ContextMT;
class ProgramMT;

class ShaderMT : public ShaderBase
{
public:
    class ResourceBindingMT : public ResourceBindingBase
    {
    public:
        struct Settings
        {
            ResourceBindingBase::Settings base;
            MTLArgumentType               argument_type;
            uint32_t                      argument_index;
        };
        
        ResourceBindingMT(ContextBase& context, const Settings& settings);
        ResourceBindingMT(const ResourceBindingMT& other) = default;
        
        // ResourceBinding interface
        void SetResource(const Resource::Ptr& sp_resource) override;
        uint32_t GetResourceCount() const override { return 1; }
        
        // ResourceBindingBase interface
        DescriptorHeap::Type GetDescriptorHeapType() const override;
        
        const Settings& GetSettings() const noexcept { return m_settings; }
        
    protected:
        const Settings m_settings;
    };
    
    ShaderMT(Shader::Type shader_type, ContextBase& context, const Settings& settings);
    ~ShaderMT() override;
    
    // ShaderBase interface
    ResourceBindings GetResourceBindings(const std::set<std::string>& constant_argument_names) const override;
    
    id<MTLFunction>& GetNativeFunction() noexcept                           { return m_mtl_function; }
    MTLVertexDescriptor* GetNativeVertexDescriptor(const ProgramMT& program) const;
    void SetNativeArguments(NSArray<MTLArgument*>* mtl_arguments) noexcept  { m_mtl_arguments = mtl_arguments; }

protected:
    class LibraryMT
    {
    public:
        LibraryMT(ContextMT& metal_context);
        ~LibraryMT();

        id<MTLLibrary>& Get() noexcept { return m_mtl_library; }

    private:
        id<MTLLibrary> m_mtl_library;
    };
    
    ContextMT& GetContextMT() noexcept;
    LibraryMT& GetLibraryMT() noexcept;

    id<MTLFunction>        m_mtl_function;
    NSArray<MTLArgument*>* m_mtl_arguments = nil;
};

} // namespace Graphics
} // namespace Methane
