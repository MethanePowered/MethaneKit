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

FILE: AsteroidsArray.cpp
Random generated asteroids array with uber mesh and textures ready for rendering.

******************************************************************************/

#include "AsteroidsArray.h"

#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <future>
#include <cmath>

namespace Methane::Samples
{

static hlslpp::float3 GetRandomDirection(std::mt19937& rng)
{
    META_FUNCTION_TASK();

    std::normal_distribution<float> distribution;
    hlslpp::float3 direction;
    do
    {
        direction = { distribution(rng), distribution(rng), distribution(rng) };
    }
    while (static_cast<float>(hlslpp::length(direction)) <= std::numeric_limits<float>::min());
    return hlslpp::normalize(direction);
}

AsteroidsArray::UberMesh::UberMesh(tf::Executor& parallel_executor, uint32_t instance_count, uint32_t subdivisions_count, uint32_t random_seed)
    : gfx::UberMesh<Asteroid::Vertex>(Asteroid::Vertex::layout)
    , m_instance_count(instance_count)
    , m_subdivisions_count(subdivisions_count)
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsArray::UberMesh::UberMesh");

    m_depth_ranges.reserve(static_cast<size_t>(m_instance_count) * m_subdivisions_count);

    std::mt19937 rng(random_seed); // NOSONAR - using pseudorandom generator is safe here
    TracyLockable(std::mutex, data_mutex)

    for (uint32_t subdivision_index = 0; subdivision_index < m_subdivisions_count; ++subdivision_index)
    {
        Asteroid::Mesh base_mesh(subdivision_index, false);
        base_mesh.Spherify();

        tf::Taskflow task_flow;
        task_flow.for_each_index(0U, m_instance_count, 1U,
            [this, &rng, &data_mutex, &base_mesh](const uint32_t)
            {
                Asteroid::Mesh asteroid_mesh(base_mesh);
                asteroid_mesh.Randomize(rng()); // NOSONAR

                std::scoped_lock lock_guard(data_mutex);
                m_depth_ranges.emplace_back(asteroid_mesh.GetDepthRange());
                AddSubMesh(asteroid_mesh, false);
            }
        );
        parallel_executor.run(task_flow).get();
    }
}

uint32_t AsteroidsArray::UberMesh::GetSubsetIndex(uint32_t instance_index, uint32_t subdivision_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(instance_index, m_instance_count);
    META_CHECK_ARG_LESS(subdivision_index, m_subdivisions_count);

    return subdivision_index * m_instance_count + instance_index;
}

uint32_t AsteroidsArray::UberMesh::GetSubsetSubdivision(uint32_t subset_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(subset_index, GetSubsetCount());

    const uint32_t subdivision_index = subset_index / m_instance_count;
    META_CHECK_ARG_LESS(subdivision_index, m_subdivisions_count);

    return subdivision_index;
}

const Asteroid::Mesh::DepthRange& AsteroidsArray::UberMesh::GetSubsetDepthRange(uint32_t subset_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(subset_index, GetSubsetCount());

    assert(subset_index < m_depth_ranges.size());
    return m_depth_ranges[subset_index];
}

AsteroidsArray::ContentState::ContentState(tf::Executor& parallel_executor, const Settings& settings)
    : uber_mesh(parallel_executor, settings.unique_mesh_count, settings.subdivisions_count, settings.random_seed)
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsArray::ContentState::ContentState");

    std::mt19937 rng(settings.random_seed); // NOSONAR - using pseudorandom generator is safe here

    // Randomly generate perlin-noise textures
    std::uniform_real_distribution<float> noise_gain_distribution(0.2F, 0.8F);
    std::uniform_real_distribution<float> noise_fractal_distribution(0.3F, 0.7F);
    std::uniform_real_distribution<float> noise_lacunarity_distribution(1.5F, 2.5F);
    std::uniform_real_distribution<float> noise_scale_distribution(0.05F, 0.1F);
    std::uniform_real_distribution<float> noise_strength_distribution(0.8F, 1.0F);

    texture_array_subresources.resize(settings.textures_count);
    tf::Taskflow task_flow;
    task_flow.for_each(texture_array_subresources.begin(), texture_array_subresources.end(),
        [&rng, &noise_gain_distribution, &noise_fractal_distribution, &noise_lacunarity_distribution, &noise_scale_distribution, &noise_strength_distribution, &settings]
        (gfx::Resource::SubResources& sub_resources)
        {
            sub_resources = Asteroid::GenerateTextureArraySubresources(settings.texture_dimensions, 3,
                                                                       Asteroid::TextureNoiseParameters{
                                                                           static_cast<uint32_t>(rng()), //NOSONAR
                                                                           noise_gain_distribution(rng),
                                                                           noise_fractal_distribution(rng),
                                                                           noise_lacunarity_distribution(rng),
                                                                           noise_scale_distribution(rng),
                                                                           noise_strength_distribution(rng)
                                                                       });
        }
    );
    parallel_executor.run(task_flow).get();

    // Randomly distribute textures between uber-mesh subsets
    std::uniform_int_distribution<uint32_t> textures_distribution(0U, settings.textures_count - 1);
    mesh_subset_texture_indices.resize(static_cast<size_t>(settings.unique_mesh_count) * settings.subdivisions_count);
    for (uint32_t& mesh_subset_texture_index : mesh_subset_texture_indices)
    {
        mesh_subset_texture_index = textures_distribution(rng);
    }

    // Randomly generate parameters of each asteroid in array
    const float    orbit_radius = settings.orbit_radius_ratio * settings.scale;
    const float    disc_radius  = settings.disc_radius_ratio  * settings.scale;

    std::normal_distribution<float>         normal_distribution;
    std::uniform_int_distribution<uint32_t> mesh_distribution(0U, settings.unique_mesh_count - 1);
    std::uniform_int_distribution<uint32_t> colors_distribution(0, static_cast<uint32_t>(Asteroid::color_schema_size - 1));
    std::uniform_real_distribution<float>   scale_distribution(settings.min_asteroid_scale_ratio, settings.max_asteroid_scale_ratio);
    std::uniform_real_distribution<float>   scale_proportion_distribution(0.8F, 1.2F);
    std::uniform_real_distribution<float>   spin_velocity_distribution(-1.7F, 1.7F);
    std::uniform_real_distribution<float>   orbit_velocity_distribution(1.5F, 5.F);
    std::normal_distribution<float>         orbit_radius_distribution(orbit_radius, 0.6F * disc_radius);
    std::normal_distribution<float>         orbit_height_distribution(0.0F, 0.4F * disc_radius);

    parameters.reserve(settings.instance_count);

    for (uint32_t asteroid_index = 0; asteroid_index < settings.instance_count; ++asteroid_index)
    {
        const uint32_t      asteroid_mesh_index   = mesh_distribution(rng);
        const float         asteroid_orbit_radius = orbit_radius_distribution(rng);
        const float         asteroid_orbit_height = orbit_height_distribution(rng);
        const float         asteroid_scale_ratio  = scale_distribution(rng);
        const float         asteroid_scale        = asteroid_scale_ratio * settings.scale;
        const hlslpp::float3 asteroid_scale_ratios = hlslpp::float3(scale_proportion_distribution(rng),
                                                                    scale_proportion_distribution(rng),
                                                                    scale_proportion_distribution(rng)) * asteroid_scale_ratio;

        hlslpp::float4x4 scale_translate_matrix = hlslpp::mul(
            hlslpp::float4x4::scale(asteroid_scale_ratios * settings.scale),
            hlslpp::float4x4::translation(asteroid_orbit_radius, asteroid_orbit_height, 0.F)
        );
        Asteroid::Colors asteroid_colors = normal_distribution(rng) <= 1.F
                                         ? Asteroid::GetAsteroidIceColors(colors_distribution(rng), colors_distribution(rng))
                                         : Asteroid::GetAsteroidRockColors(colors_distribution(rng), colors_distribution(rng));

        parameters.emplace_back(
            Asteroid::Parameters
            {
                asteroid_index,
                asteroid_mesh_index,
                settings.textures_array_enabled ? textures_distribution(rng) : 0U,
                std::move(asteroid_colors),
                std::move(scale_translate_matrix),
                GetRandomDirection(rng),
                asteroid_scale,
                orbit_velocity_distribution(rng) / (asteroid_scale * asteroid_orbit_radius),
                spin_velocity_distribution(rng)  / asteroid_scale,
                gfx::ConstFloat::Pi * normal_distribution(rng),
                gfx::ConstFloat::Pi * normal_distribution(rng) * 2.F
            }
        );
    }
}

AsteroidsArray::AsteroidsArray(gfx::CommandQueue& render_cmd_queue, gfx::RenderPattern& render_pattern, const Settings& settings)
    : AsteroidsArray(render_cmd_queue, render_pattern, settings, *std::make_shared<ContentState>(render_pattern.GetRenderContext().GetParallelExecutor(), settings))
{
    META_FUNCTION_TASK();
}

AsteroidsArray::AsteroidsArray(gfx::CommandQueue& render_cmd_queue, gfx::RenderPattern& render_pattern, const Settings& settings, ContentState& state)
    : BaseBuffers(render_cmd_queue, state.uber_mesh, "Asteroids Array")
    , m_settings(settings)
    , m_render_cmd_queue_ptr(std::dynamic_pointer_cast<gfx::CommandQueue>(render_cmd_queue.GetPtr()))
    , m_content_state_ptr(state.shared_from_this())
    , m_mesh_subset_by_instance_index(m_settings.instance_count, 0U)
    , m_min_mesh_lod_screen_size_log_2(std::log2(m_settings.mesh_lod_min_screen_size))
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsArray::AsteroidsArray");

    const gfx::RenderContext& context = render_pattern.GetRenderContext();
    const size_t textures_array_size = m_settings.textures_array_enabled ? m_settings.textures_count : 1;
    const gfx::Shader::MacroDefinitions macro_definitions{ { "TEXTURES_COUNT", std::to_string(textures_array_size) } };

    gfx::RenderState::Settings state_settings;
    state_settings.program_ptr = gfx::Program::Create(context,
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(context, { Data::ShaderProvider::Get(), { "Asteroids", "AsteroidVS" }, macro_definitions }),
                gfx::Shader::CreatePixel( context, { Data::ShaderProvider::Get(), { "Asteroids", "AsteroidPS" }, macro_definitions }),
            },
            gfx::Program::InputBufferLayouts
            {
                gfx::Program::InputBufferLayout { state.uber_mesh.GetVertexLayout().GetSemantics() }
            },
            gfx::Program::ArgumentAccessors
            {
                { { gfx::Shader::Type::All,    "g_mesh_uniforms"  }, gfx::Program::ArgumentAccessor::Type::Mutable, true },
                { { gfx::Shader::Type::All,    "g_scene_uniforms" }, gfx::Program::ArgumentAccessor::Type::FrameConstant },
                { { gfx::Shader::Type::Pixel,  "g_constants"      }, gfx::Program::ArgumentAccessor::Type::Constant      },
                { { gfx::Shader::Type::Pixel,  "g_texture_sampler"}, gfx::Program::ArgumentAccessor::Type::Constant      },
                { { gfx::Shader::Type::Pixel,  "g_face_textures"  }, m_settings.textures_array_enabled
                                                                     ? gfx::Program::ArgumentAccessor::Type::Constant
                                                                     : gfx::Program::ArgumentAccessor::Type::Mutable     },
            },
            render_pattern.GetAttachmentFormats()
        }
    );
    state_settings.program_ptr->SetName("Asteroid Shaders");
    state_settings.render_pattern_ptr = std::dynamic_pointer_cast<gfx::RenderPattern>(render_pattern.GetPtr());
    state_settings.depth.enabled = true;
    state_settings.depth.compare = m_settings.depth_reversed ? gfx::Compare::GreaterEqual : gfx::Compare::Less;
    
    m_render_state_ptr = gfx::RenderState::Create(context, state_settings);
    m_render_state_ptr->SetName("Asteroids Render State");

    SetInstanceCount(m_settings.instance_count);

    // Create texture arrays initialized with sub-resources data
    uint32_t texture_index = 0U;
    m_unique_textures.reserve(m_settings.textures_count);
    for(const gfx::Resource::SubResources& texture_subresources : m_content_state_ptr->texture_array_subresources)
    {
        m_unique_textures.emplace_back(gfx::Texture::CreateImage(context, m_settings.texture_dimensions, static_cast<uint32_t>(texture_subresources.size()), gfx::PixelFormat::RGBA8Unorm, true));
        m_unique_textures.back()->SetData(texture_subresources, *m_render_cmd_queue_ptr);
        m_unique_textures.back()->SetName(fmt::format("Asteroid Texture {:d}", texture_index));
        texture_index++;
    }

    // Distribute textures between unique mesh subsets
    for (uint32_t subset_index = 0; subset_index < m_content_state_ptr->mesh_subset_texture_indices.size(); ++subset_index)
    {
        const uint32_t subset_texture_index = m_content_state_ptr->mesh_subset_texture_indices[subset_index];
        META_CHECK_ARG_LESS(subset_texture_index, m_unique_textures.size());
        SetSubsetTexture(m_unique_textures[subset_texture_index], subset_index);
    }
    
    m_texture_sampler_ptr = gfx::Sampler::Create(context, {
        gfx::Sampler::Filter(gfx::Sampler::Filter::MinMag::Linear),
        gfx::Sampler::Address(gfx::Sampler::Address::Mode::ClampToZero)
    });
    m_texture_sampler_ptr->SetName("Asteroid Texture Sampler");

    // Initialize default uniforms to be ready to render right aways
    Update(0.0, 0.0);
}
    
Ptrs<gfx::ProgramBindings> AsteroidsArray::CreateProgramBindings(const Ptr<gfx::Buffer>& constants_buffer_ptr,
                                                                 const Ptr<gfx::Buffer>& scene_uniforms_buffer_ptr,
                                                                 const Ptr<gfx::Buffer>& asteroids_uniforms_buffer_ptr,
                                                                 Data::Index frame_index) const
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsArray::CreateProgramBindings");

    Ptrs<gfx::ProgramBindings> program_bindings_array;
    if (m_settings.instance_count == 0)
        return program_bindings_array;

    const Data::Size uniform_data_size = MeshBuffers::GetAlignedUniformSize();
    const gfx::Resource::Views face_texture_locations = m_settings.textures_array_enabled
                                                          ? gfx::Resource::CreateViews(m_unique_textures)
                                                          : gfx::Resource::Views{ { GetInstanceTexture() } };
    
    program_bindings_array.resize(m_settings.instance_count);
    program_bindings_array[0] = gfx::ProgramBindings::Create(m_render_state_ptr->GetSettings().program_ptr, {
        { { gfx::Shader::Type::All,    "g_mesh_uniforms"  }, { { *asteroids_uniforms_buffer_ptr, GetUniformsBufferOffset(0), uniform_data_size } } },
        { { gfx::Shader::Type::All,    "g_scene_uniforms" }, { { *scene_uniforms_buffer_ptr } } },
        { { gfx::Shader::Type::Pixel,  "g_constants"      }, { { *constants_buffer_ptr      } } },
        { { gfx::Shader::Type::Pixel,  "g_face_textures"  },     face_texture_locations         },
        { { gfx::Shader::Type::Pixel,  "g_texture_sampler"}, { { *m_texture_sampler_ptr     } } },
    }, frame_index);
    program_bindings_array[0]->SetName(fmt::format("Asteroids[0] Bindings {}", frame_index));

    tf::Taskflow task_flow;
    task_flow.for_each_index(1U, m_settings.instance_count, 1U,
        [this, &program_bindings_array, &asteroids_uniforms_buffer_ptr, uniform_data_size, frame_index](const uint32_t asteroid_index)
        {
            const Data::Size asteroid_uniform_offset = GetUniformsBufferOffset(asteroid_index);
            META_CHECK_ARG_EQUAL(asteroid_uniform_offset % 256, 0);
            gfx::ProgramBindings::ResourceViewsByArgument set_resource_view_by_argument{
                { { gfx::Shader::Type::All, "g_mesh_uniforms" }, { { *asteroids_uniforms_buffer_ptr, asteroid_uniform_offset, uniform_data_size } } },
            };
            if (!m_settings.textures_array_enabled)
            {
                set_resource_view_by_argument.insert(
                    { { gfx::Shader::Type::Pixel, "g_face_textures" }, { { GetInstanceTexture(asteroid_index) } } }
                );
            }
            program_bindings_array[asteroid_index] = gfx::ProgramBindings::CreateCopy(*program_bindings_array[0], set_resource_view_by_argument, frame_index);
            program_bindings_array[asteroid_index]->SetName(fmt::format("Asteroids[{}] Bindings {}", asteroid_index, frame_index));
        }
    );
    GetContext().GetParallelExecutor().run(task_flow).get();

    return program_bindings_array;
}

bool AsteroidsArray::Update(double elapsed_seconds, double /*delta_seconds*/)
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsArray::Update");

    const float elapsed_radians = gfx::ConstFloat::Pi * static_cast<float>(elapsed_seconds);

    tf::Taskflow update_task_flow;
    update_task_flow.for_each(m_content_state_ptr->parameters.begin(), m_content_state_ptr->parameters.end(),
        [this, elapsed_radians](const Asteroid::Parameters& asteroid_parameters)
        {
            UpdateAsteroidUniforms(asteroid_parameters, m_settings.view_camera.GetOrientation().eye, elapsed_radians);
        }
    );

    GetContext().GetParallelExecutor().run(update_task_flow).get();
    return true;
}

void AsteroidsArray::Draw(gfx::RenderCommandList &cmd_list,
                          const gfx::InstancedMeshBufferBindings& buffer_bindings,
                          gfx::ViewState& view_state)
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsArray::Draw");
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Asteroids rendering");

    META_CHECK_ARG_NOT_NULL(buffer_bindings.uniforms_buffer_ptr);
    META_CHECK_ARG_GREATER_OR_EQUAL(buffer_bindings.uniforms_buffer_ptr->GetDataSize(), GetUniformsBufferSize());

    // Upload uniforms buffer data to GPU asynchronously while encoding drawing commands on CPU
    auto uniforms_update_future = std::async([this, &buffer_bindings]() {
        buffer_bindings.uniforms_buffer_ptr->SetData(GetFinalPassUniformsSubresources(), *m_render_cmd_queue_ptr);
    });

    cmd_list.ResetWithState(*m_render_state_ptr, s_debug_group.get());
    cmd_list.SetViewState(view_state);

    META_CHECK_ARG_EQUAL(buffer_bindings.program_bindings_per_instance.size(), m_settings.instance_count);
    BaseBuffers::Draw(
        cmd_list, buffer_bindings.program_bindings_per_instance,
        gfx::ProgramBindings::ApplyBehavior::ConstantOnce, 0, // Constant bindings are applied once mutable always, resource barriers are not set to reduce overhead
        true,   // Bound resources are not retained by command lists to reduce overhead from the huge amount of bindings
        false   // Do not set resource barriers for Vertex and Index buffers since their state does not change and to reduce runtime overhead
    );

    // Make sure that uniforms have finished uploading to GPU
    uniforms_update_future.wait();
}

void AsteroidsArray::DrawParallel(gfx::ParallelRenderCommandList& parallel_cmd_list,
                                  const gfx::InstancedMeshBufferBindings& buffer_bindings,
                                  gfx::ViewState& view_state)
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsArray::DrawParallel");
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Parallel Asteroids rendering");

    META_CHECK_ARG_NOT_NULL(buffer_bindings.uniforms_buffer_ptr);
    META_CHECK_ARG_GREATER_OR_EQUAL(buffer_bindings.uniforms_buffer_ptr->GetDataSize(), GetUniformsBufferSize());

    // Upload uniforms buffer data to GPU asynchronously while encoding drawing commands on CPU
    auto uniforms_update_future = std::async([this, &buffer_bindings]() {
        buffer_bindings.uniforms_buffer_ptr->SetData(GetFinalPassUniformsSubresources(), *m_render_cmd_queue_ptr);
    });

    parallel_cmd_list.ResetWithState(*m_render_state_ptr, s_debug_group.get());
    parallel_cmd_list.SetViewState(view_state);

    META_CHECK_ARG_EQUAL(buffer_bindings.program_bindings_per_instance.size(), m_settings.instance_count);
    BaseBuffers::DrawParallel(
        parallel_cmd_list, buffer_bindings.program_bindings_per_instance,
        gfx::ProgramBindings::ApplyBehavior::ConstantOnce, // Constant bindings are applied once, mutable always, resource barriers are not set to reduce overhead
        true,   // Bound resources are not retained by command lists to reduce overhead from the huge amount of bindings
        false   // Do not set resource barriers for Vertex and Index buffers since their state does not change and to reduce runtime overhead
    );
    uniforms_update_future.wait();
}

float AsteroidsArray::GetMinMeshLodScreenSize() const
{
    META_FUNCTION_TASK();
    return std::pow(2.F, m_min_mesh_lod_screen_size_log_2);
}

void AsteroidsArray::SetMinMeshLodScreenSize(float mesh_lod_min_screen_size)
{
    META_FUNCTION_TASK();
    m_min_mesh_lod_screen_size_log_2 = std::log2(mesh_lod_min_screen_size);
}

uint32_t AsteroidsArray::GetSubsetByInstanceIndex(uint32_t instance_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(instance_index, m_mesh_subset_by_instance_index.size());
    return m_mesh_subset_by_instance_index[instance_index];
}

void AsteroidsArray::UpdateAsteroidUniforms(const Asteroid::Parameters& asteroid_parameters, const hlslpp::float3& eye_position, float elapsed_radians)
{
    META_FUNCTION_TASK();

    const float spin_angle_rad  = asteroid_parameters.spin_angle_rad  + asteroid_parameters.spin_speed  * elapsed_radians;
    const float orbit_angle_rad = asteroid_parameters.orbit_angle_rad - asteroid_parameters.orbit_speed * elapsed_radians;

    const hlslpp::float4x4 spin_rotation_matrix  = hlslpp::float4x4::rotation_axis(asteroid_parameters.spin_axis, spin_angle_rad);
    const hlslpp::float4x4 orbit_rotation_matrix = hlslpp::float4x4::rotation_y(orbit_angle_rad);

    const hlslpp::float4x4 model_matrix = hlslpp::mul(hlslpp::mul(spin_rotation_matrix, asteroid_parameters.scale_translate_matrix), orbit_rotation_matrix);
    const hlslpp::float3   asteroid_position(model_matrix._m30, model_matrix._m31, model_matrix._m32);
    const float            distance_to_eye            = hlslpp::length(eye_position - asteroid_position);
    const float            relative_screen_size_log_2 = std::log2(asteroid_parameters.scale / std::sqrt(distance_to_eye));

    const float    mesh_subdiv_float        = std::roundf(relative_screen_size_log_2 - m_min_mesh_lod_screen_size_log_2);
    const uint32_t mesh_subdivision_index   = std::min(m_settings.subdivisions_count - 1, static_cast<uint32_t>(std::max(0.0F, mesh_subdiv_float)));
    const uint32_t mesh_subset_index        = m_content_state_ptr->uber_mesh.GetSubsetIndex(asteroid_parameters.mesh_instance_index, mesh_subdivision_index);
    const auto&   [mesh_depth_min, mesh_depth_max] = m_content_state_ptr->uber_mesh.GetSubsetDepthRange(mesh_subset_index);
    const Asteroid::Colors& asteroid_colors = m_mesh_lod_coloring_enabled
                                            ? Asteroid::GetAsteroidLodColors(mesh_subdivision_index)
                                            : asteroid_parameters.colors;

    m_mesh_subset_by_instance_index[asteroid_parameters.index] = mesh_subset_index;

    SetFinalPassUniforms(
        hlslpp::AsteroidUniforms
        {
            hlslpp::transpose(model_matrix),
            asteroid_colors.deep.AsVector(),
            asteroid_colors.shallow.AsVector(),
            mesh_depth_min,
            mesh_depth_max,
            asteroid_parameters.texture_index
        },
        asteroid_parameters.index
    );
}

} // namespace Methane::Samples
