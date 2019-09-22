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

FILE: Methane/Graphics/RenderState.h
Methane render state interface: specifies configuration of the graphics pipeline.

******************************************************************************/

#pragma once

#include "Object.h"
#include "Program.h"
#include "Types.h"

#include <memory>
#include <string>

namespace Methane::Graphics
{

struct Context;
struct RenderCommandList;

struct RenderState : virtual Object
{
public:
    using Ptr = std::shared_ptr<RenderState>;
    using WeakPtr = std::weak_ptr<RenderState>;
    
    struct Rasterizer
    {
        enum class CullMode : uint32_t
        {
            None = 0,
            Back,
            Front,
        };
        
        enum class FillMode : uint32_t
        {
            Solid = 0,
            Wireframe,
        };
        
        bool     is_front_counter_clockwise = false;
        CullMode cull_mode                  = CullMode::Back;
        FillMode fill_mode                  = FillMode::Solid;
        uint32_t sample_count               = 1;
        bool     alpha_to_coverage_enabled  = false;
        bool     alpha_to_one_enabled       = false;
    };
    
    struct Depth
    {
        bool    enabled = false;
        Compare compare = Compare::Less;
    };
    
    struct Stencil
    {
        enum class Operation : uint32_t
        {
            Keep = 0,
            Zero,
            Replace,
            Invert,
            IncrementClamp,
            DecrementClamp,
            IncrementWrap,
            DecrementWrap,
        };

        struct FaceOperations
        {
            Operation stencil_failure    = Operation::Keep;
            Operation stencil_pass       = Operation::Keep; // DX only
            Operation depth_failure      = Operation::Keep;
            Operation depth_stencil_pass = Operation::Keep; // Metal only
            Compare   compare            = Compare::Always;
        };
        
        bool           enabled           = false;
        uint32_t       read_mask         = static_cast<uint32_t>(~0x0);
        uint32_t       write_mask        = static_cast<uint32_t>(~0x0);
        FaceOperations front_face;
        FaceOperations back_face;
    };

    struct Settings
    {
        Program::Ptr sp_program;
        Viewports    viewports;
        ScissorRects scissor_rects;
        Rasterizer   rasterizer;
        Depth        depth;
        Stencil      stencil;
    };

    // Create RenderState instance
    static Ptr Create(Context& context, const Settings& state_settings);

    // RenderState interface
    virtual const Settings& GetSettings() const = 0;
    virtual void Reset(const Settings& settings) = 0;
    virtual void SetViewports(const Viewports& viewports) = 0;
    virtual void SetScissorRects(const ScissorRects& scissor_rects) = 0;

    virtual ~RenderState() = default;
};

} // namespace Methane::Graphics
