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

#include <Methane/Graphics/PerlinNoise.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Data/Math.hpp>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <cmath>

namespace Methane::Samples
{

static gfx::Point3f GetRandomDirection(std::mt19937& rng)
{
    META_FUNCTION_TASK();

    std::normal_distribution<float> distribution;
    gfx::Point3f direction;
    do
    {
        direction = { distribution(rng), distribution(rng), distribution(rng) };
    }
    while (direction.length_squared() <= std::numeric_limits<float>::min());
    return gfx::Point3f(cml::normalize(direction));
}

AsteroidsArray::UberMesh::UberMesh(tf::Executor& parallel_executor, uint32_t instance_count, uint32_t subdivisions_count, uint32_t random_seed)
    : gfx::UberMesh<Asteroid::Vertex>(Asteroid::Vertex::layout)
    , m_instance_count(instance_count)
    , m_subdivisions_count(subdivisions_count)
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsArray::UberMesh::UberMesh");

    std::mt19937 rng(random_seed);

    m_depth_ranges.reserve(static_cast<size_t>(m_instance_count) * m_subdivisions_count);

    TracyLockable(std::mutex, data_mutex);
    for (uint32_t subdivision_index = 0; subdivision_index < m_subdivisions_count; ++subdivision_index)
    {
        Asteroid::Mesh base_mesh(subdivision_index, false);
        base_mesh.Spherify();

        tf::Taskflow task_flow;
        task_flow.for_each_index_guided(0, static_cast<int>(m_instance_count), 1,
            [&](const int)
            {
                Asteroid::Mesh asteroid_mesh(base_mesh);
                asteroid_mesh.Randomize(rng());

                std::lock_guard<LockableBase(std::mutex)> lock_guard(data_mutex);
                m_depth_ranges.emplace_back(asteroid_mesh.GetDepthRange());
                AddSubMesh(asteroid_mesh, false);
            },
            Data::GetParallelChunkSizeAsInt(m_instance_count, 5)
        );
        parallel_executor.run(task_flow).get();
    }
}

uint32_t AsteroidsArray::UberMesh::GetSubsetIndex(uint32_t instance_index, uint32_t subdivision_index)
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

const gfx::Vector2f& AsteroidsArray::UberMesh::GetSubsetDepthRange(uint32_t subset_index) const
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

    std::mt19937 rng(settings.random_seed);

    // Randomly generate perlin-noise textures
    std::normal_distribution<float>       noise_persistence_distribution(0.9F, 0.2F);
    std::uniform_real_distribution<float> noise_scale_distribution(0.05F, 0.1F);

    texture_array_subresources.resize(settings.textures_count);
    tf::Taskflow task_flow;
    task_flow.for_each_guided(texture_array_subresources.begin(), texture_array_subresources.end(),
        [&](gfx::Resource::SubResources& sub_resources)
        {
            Asteroid::TextureNoiseParameters noise_parameters{
                static_cast<uint32_t>(rng()),
                noise_persistence_distribution(rng),
                noise_scale_distribution(rng),
                1.5F
            };
            sub_resources = Asteroid::GenerateTextureArraySubresources(settings.texture_dimensions, 3, noise_parameters);
        },
        Data::GetParallelChunkSizeAsInt(texture_array_subresources.size(), 5)
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
        const gfx::Vector3f asteroid_scale_ratios = gfx::Vector3f(scale_proportion_distribution(rng),
                                                                  scale_proportion_distribution(rng),
                                                                  scale_proportion_distribution(rng)) * asteroid_scale_ratio;

        gfx::Matrix44f translation_matrix;
        cml::matrix_translation(translation_matrix, asteroid_orbit_radius, asteroid_orbit_height, 0.F);

        gfx::Matrix44f scale_matrix;
        cml::matrix_scale(scale_matrix, asteroid_scale_ratios * settings.scale);

        gfx::Matrix44f scale_translate_matrix = scale_matrix * translation_matrix;

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
                cml::constants<float>::pi() * normal_distribution(rng),
                cml::constants<float>::pi() * normal_distribution(rng) * 2.F
            }
        );
    }
}

AsteroidsArray::AsteroidsArray(gfx::RenderContext& context, Settings settings)
    : AsteroidsArray(context, settings, *std::make_shared<ContentState>(context.GetParallelExecutor(), settings))
{
    META_FUNCTION_TASK();
}

AsteroidsArray::AsteroidsArray(gfx::RenderContext& context, Settings settings, ContentState& state)
    : BaseBuffers(context, state.uber_mesh, "Asteroids Array")
    , m_settings(std::move(settings))
    , m_content_state_ptr(state.shared_from_this())
    , m_mesh_subset_by_instance_index(m_settings.instance_count, 0U)
    , m_min_mesh_lod_screen_size_log_2(std::log2(m_settings.mesh_lod_min_screen_size))
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsArray::AsteroidsArray");
    
    const gfx::RenderContext::Settings& context_settings = context.GetSettings();

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
            gfx::Program::ArgumentDescriptions
            {
                { { gfx::Shader::Type::All,    "g_mesh_uniforms"  }, gfx::Program::Argument::Modifiers::Addressable },
                { { gfx::Shader::Type::Pixel,  "g_scene_uniforms" }, gfx::Program::Argument::Modifiers::None        },
                { { gfx::Shader::Type::Pixel,  "g_constants"      }, gfx::Program::Argument::Modifiers::Constant    },
                { { gfx::Shader::Type::Pixel,  "g_texture_sampler"}, gfx::Program::Argument::Modifiers::Constant    },
                { { gfx::Shader::Type::Pixel,  "g_face_textures"  }, m_settings.textures_array_enabled
                                                                     ? gfx::Program::Argument::Modifiers::Constant
                                                                     : gfx::Program::Argument::Modifiers::None      },
            },
            gfx::PixelFormats
            {
                context_settings.color_format
            },
            context_settings.depth_stencil_format
        }
    );
    state_settings.program_ptr->SetName("Asteroid Shaders");
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
        m_unique_textures.back()->SetData(texture_subresources);
        m_unique_textures.back()->SetName("Asteroid Texture " + std::to_string(texture_index++));
    }

    // Distribute textures between unique mesh subsets
    for (uint32_t subset_index = 0; subset_index < m_content_state_ptr->mesh_subset_texture_indices.size(); ++subset_index)
    {
        const uint32_t subset_texture_index = m_content_state_ptr->mesh_subset_texture_indices[subset_index];
        META_CHECK_ARG_LESS(subset_texture_index, m_unique_textures.size());
        SetSubsetTexture(m_unique_textures[subset_texture_index], subset_index);
    }
    
    m_texture_sampler_ptr = gfx::Sampler::Create(context, {
        { gfx::Sampler::Filter::MinMag::Linear     },
        { gfx::Sampler::Address::Mode::ClampToZero }
    });
    m_texture_sampler_ptr->SetName("Asteroid Texture Sampler");

    // Initialize default uniforms to be ready to render right aways
    Update(0.0, 0.0);
}
    
Ptrs<gfx::ProgramBindings> AsteroidsArray::CreateProgramBindings(const Ptr<gfx::Buffer>& constants_buffer_ptr,
                                                                 const Ptr<gfx::Buffer>& scene_uniforms_buffer_ptr,
                                                                 const Ptr<gfx::Buffer>& asteroids_uniforms_buffer_ptr)
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsArray::CreateProgramBindings");

    Ptrs<gfx::ProgramBindings> program_bindings_array;
    if (m_settings.instance_count == 0)
        return program_bindings_array;

    const gfx::Resource::Locations face_texture_locations = m_settings.textures_array_enabled
                                                          ? gfx::Resource::CreateLocations(m_unique_textures)
                                                          : gfx::Resource::Locations{ { GetInstanceTexturePtr(0) } };
    
    program_bindings_array.resize(m_settings.instance_count);
    program_bindings_array[0] = gfx::ProgramBindings::Create(m_render_state_ptr->GetSettings().program_ptr, {
        { { gfx::Shader::Type::All,    "g_mesh_uniforms"  }, { { asteroids_uniforms_buffer_ptr, GetUniformsBufferOffset(0) } } },
        { { gfx::Shader::Type::Pixel,  "g_scene_uniforms" }, { { scene_uniforms_buffer_ptr } } },
        { { gfx::Shader::Type::Pixel,  "g_constants"      }, { { constants_buffer_ptr      } } },
        { { gfx::Shader::Type::Pixel,  "g_face_textures"  },     face_texture_locations        },
        { { gfx::Shader::Type::Pixel,  "g_texture_sampler"}, { { m_texture_sampler_ptr     } } },
    });

    tf::Taskflow task_flow;
    task_flow.for_each_index_guided(1, static_cast<int>(m_settings.instance_count), 1,
        [&](const int asteroid_index)
        {
            const Data::Size asteroid_uniform_offset = GetUniformsBufferOffset(static_cast<uint32_t>(asteroid_index));
            gfx::ProgramBindings::ResourceLocationsByArgument set_resource_location_by_argument{
                { { gfx::Shader::Type::All, "g_mesh_uniforms"  }, { { asteroids_uniforms_buffer_ptr, asteroid_uniform_offset } } },
            };
            if (!m_settings.textures_array_enabled)
            {
                set_resource_location_by_argument.insert(
                    { { gfx::Shader::Type::Pixel, "g_face_textures"  }, { { GetInstanceTexturePtr(static_cast<uint32_t>(asteroid_index)) } } }
                );
            }
            program_bindings_array[asteroid_index] = gfx::ProgramBindings::CreateCopy(*program_bindings_array[0], set_resource_location_by_argument);
        },
        Data::GetParallelChunkSizeAsInt(m_settings.instance_count, 5)
    );
    GetRenderContext().GetParallelExecutor().run(task_flow).get();
    
    return program_bindings_array;
}

bool AsteroidsArray::Update(double elapsed_seconds, double /*delta_seconds*/)
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsArray::Update");

    const gfx::Vector3f&  eye_position     = m_settings.view_camera.GetOrientation().eye;
    const gfx::Matrix44f& view_proj_matrix = m_settings.view_camera.GetViewProjMatrix();
    const float elapsed_radians = cml::constants<float>::pi()* static_cast<float>(elapsed_seconds);

    tf::Taskflow update_task_flow;
    update_task_flow.for_each_guided(m_content_state_ptr->parameters.begin(), m_content_state_ptr->parameters.end(),
        [this, &view_proj_matrix, elapsed_radians, &eye_position](Asteroid::Parameters& asteroid_parameters)
        {
            META_FUNCTION_TASK();

            const float spin_angle_rad  = asteroid_parameters.spin_angle_rad  + asteroid_parameters.spin_speed  * elapsed_radians;
            const float orbit_angle_rad = asteroid_parameters.orbit_angle_rad - asteroid_parameters.orbit_speed * elapsed_radians;

            gfx::Matrix44f spin_rotation_matrix;
            cml::matrix_rotation_axis_angle(spin_rotation_matrix, asteroid_parameters.spin_axis, spin_angle_rad);

            gfx::Matrix44f orbit_rotation_matrix;
            cml::matrix_rotation_world_y(orbit_rotation_matrix, orbit_angle_rad);

            const gfx::Matrix44f    model_matrix = spin_rotation_matrix * asteroid_parameters.scale_translate_matrix * orbit_rotation_matrix;
            const gfx::Matrix44f    mvp_matrix   = model_matrix * view_proj_matrix;

            const gfx::Vector3f     asteroid_position(model_matrix(3, 0), model_matrix(3, 1), model_matrix(3, 2));
            const float distance_to_eye            = (eye_position - asteroid_position).length();
            const float relative_screen_size_log_2 = std::log2(asteroid_parameters.scale / std::sqrt(distance_to_eye));

            const float             mesh_subdiv_float       = std::roundf(relative_screen_size_log_2 - m_min_mesh_lod_screen_size_log_2);
            const uint32_t          mesh_subdivision_index  = std::min(m_settings.subdivisions_count - 1, static_cast<uint32_t>(std::max(0.0F, mesh_subdiv_float)));
            const uint32_t          mesh_subset_index       = m_content_state_ptr->uber_mesh.GetSubsetIndex(asteroid_parameters.mesh_instance_index, mesh_subdivision_index);
            const gfx::Vector2f&    mesh_subset_depth_range = m_content_state_ptr->uber_mesh.GetSubsetDepthRange(mesh_subset_index);
            const Asteroid::Colors& asteroid_colors         = m_mesh_lod_coloring_enabled ? Asteroid::GetAsteroidLodColors(mesh_subdivision_index)
                                                                                          : asteroid_parameters.colors;

            m_mesh_subset_by_instance_index[asteroid_parameters.index] = mesh_subset_index;

            SetFinalPassUniforms(
                AsteroidUniforms
                {
                    model_matrix,
                    mvp_matrix,
                    asteroid_colors.deep,
                    asteroid_colors.shallow,
                    mesh_subset_depth_range,
                    asteroid_parameters.texture_index
                },
                asteroid_parameters.index
            );
        },
        Data::GetParallelChunkSizeAsInt(m_content_state_ptr->parameters.size(), 5)
    );

    GetRenderContext().GetParallelExecutor().run(update_task_flow).get();
    return true;
}

void AsteroidsArray::Draw(gfx::RenderCommandList &cmd_list, gfx::MeshBufferBindings& buffer_bindings, gfx::ViewState& view_state)
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsArray::Draw");
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Asteroids rendering");

    META_CHECK_ARG_NOT_NULL(buffer_bindings.uniforms_buffer_ptr);
    META_CHECK_ARG_GREATER_OR_EQUAL(buffer_bindings.uniforms_buffer_ptr->GetDataSize(), GetUniformsBufferSize());
    buffer_bindings.uniforms_buffer_ptr->SetData(GetFinalPassUniformsSubresources());

    cmd_list.ResetWithState(m_render_state_ptr, s_debug_group.get());
    cmd_list.SetViewState(view_state);

    META_CHECK_ARG_EQUAL(buffer_bindings.program_bindings_per_instance.size(), m_settings.instance_count);
    BaseBuffers::Draw(cmd_list, buffer_bindings.program_bindings_per_instance,
                      gfx::ProgramBindings::ApplyBehavior::ConstantOnce);
}

void AsteroidsArray::DrawParallel(gfx::ParallelRenderCommandList& parallel_cmd_list, gfx::MeshBufferBindings& buffer_bindings, gfx::ViewState& view_state)
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsArray::DrawParallel");
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Parallel Asteroids rendering");

    META_CHECK_ARG_NOT_NULL(buffer_bindings.uniforms_buffer_ptr);
    META_CHECK_ARG_GREATER_OR_EQUAL(buffer_bindings.uniforms_buffer_ptr->GetDataSize(), GetUniformsBufferSize());
    buffer_bindings.uniforms_buffer_ptr->SetData(GetFinalPassUniformsSubresources());

    parallel_cmd_list.ResetWithState(m_render_state_ptr, s_debug_group.get());
    parallel_cmd_list.SetViewState(view_state);

    META_CHECK_ARG_EQUAL(buffer_bindings.program_bindings_per_instance.size(), m_settings.instance_count);
    BaseBuffers::DrawParallel(parallel_cmd_list, buffer_bindings.program_bindings_per_instance,
                              gfx::ProgramBindings::ApplyBehavior::ConstantOnce);
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

} // namespace Methane::Samples
