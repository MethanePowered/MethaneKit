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

FILE: AsteroidsArray.cpp
Random generated asteroids array with uber mesh and textures ready for rendering.

******************************************************************************/

#include "AsteroidsArray.h"

#include <Methane/Graphics/Noise.hpp>
#include <Methane/Data/Parallel.hpp>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

#include <cmath>

namespace Methane::Samples
{

static gfx::Point3f GetRandomDirection(std::mt19937& rng)
{
    ITT_FUNCTION_TASK();

    std::normal_distribution<float> distribution;
    gfx::Point3f direction;
    do
    {
        direction = { distribution(rng), distribution(rng), distribution(rng) };
    }
    while (direction.length_squared() <= std::numeric_limits<float>::min());
    return cml::normalize(direction);
}

AsteroidsArray::UberMesh::UberMesh(uint32_t instance_count, uint32_t subdivisions_count, uint32_t random_seed)
    : gfx::UberMesh<Asteroid::Vertex>(Asteroid::Vertex::layout)
    , m_instance_count(instance_count)
    , m_subdivisions_count(subdivisions_count)
{
    ITT_FUNCTION_TASK();
    SCOPE_TIMER("AsteroidsArray::UberMesh::UberMesh");

    std::mt19937 rng(random_seed);

    m_depth_ranges.reserve(static_cast<size_t>(m_instance_count) * m_subdivisions_count);

    std::mutex data_mutex;
    for (uint32_t subdivision_index = 0; subdivision_index < m_subdivisions_count; ++subdivision_index)
    {
        Asteroid::Mesh base_mesh(subdivision_index, false);
        base_mesh.Spherify();

        Data::ParallelFor<uint32_t>(0u, m_instance_count, [&](uint32_t)
            {
                Asteroid::Mesh asteroid_mesh(base_mesh);
                asteroid_mesh.Randomize(rng());

                std::lock_guard<std::mutex> lock_guard(data_mutex);
                m_depth_ranges.emplace_back(asteroid_mesh.GetDepthRange());
                AddSubMesh(asteroid_mesh, false);
            });
    }
}

uint32_t AsteroidsArray::UberMesh::GetSubsetIndex(uint32_t instance_index, uint32_t subdivision_index)
{
    ITT_FUNCTION_TASK();

    if (instance_index >= m_instance_count)
        throw std::invalid_argument("Uber-mesh instance index is out of range.");

    if (subdivision_index >= m_subdivisions_count)
        throw std::invalid_argument("Uber-mesh subdivision index is out of range.");

    return subdivision_index * m_instance_count + instance_index;
}

uint32_t AsteroidsArray::UberMesh::GetSubsetSubdivision(uint32_t subset_index) const
{
    ITT_FUNCTION_TASK();

    if (subset_index >= GetSubsetCount())
        throw std::invalid_argument("Subset index is out of range.");

    const uint32_t subdivision_index = subset_index / m_instance_count;
    assert(subdivision_index < m_subdivisions_count);

    return subdivision_index;
}

const gfx::Vector2f& AsteroidsArray::UberMesh::GetSubsetDepthRange(uint32_t subset_index) const
{
    ITT_FUNCTION_TASK();

    if (subset_index >= GetSubsetCount())
        throw std::invalid_argument("Subset index is out of range.");

    assert(subset_index < m_depth_ranges.size());
    return m_depth_ranges[subset_index];
}

AsteroidsArray::ContentState::ContentState(const Settings& settings)
    : uber_mesh(settings.unique_mesh_count, settings.subdivisions_count, settings.random_seed)
{
    ITT_FUNCTION_TASK();
    SCOPE_TIMER("AsteroidsArray::ContentState::ContentState");

    std::mt19937 rng(settings.random_seed);

    // Randomly generate perlin-noise textures
    std::normal_distribution<float>       noise_persistence_distribution(0.9f, 0.2f);
    std::uniform_real_distribution<float> noise_scale_distribution(0.05f, 0.1f);

    texture_array_subresources.resize(settings.textures_count);
    Data::ParallelForEach<TextureArraySubresources::iterator, gfx::Resource::SubResources>(texture_array_subresources.begin(), texture_array_subresources.end(),
        [&](gfx::Resource::SubResources& sub_resources)
        {
            Asteroid::TextureNoiseParameters noise_parameters = {
                static_cast<uint32_t>(rng()),
                noise_persistence_distribution(rng),
                noise_scale_distribution(rng),
                1.5f
            };
            sub_resources = Asteroid::GenerateTextureArraySubresources(settings.texture_dimensions, 3, noise_parameters);
        });

    // Randomly distribute textures between uber-mesh subsets
    std::uniform_int_distribution<uint32_t> textures_distribution(0u, settings.textures_count - 1);
    mesh_subset_texture_indices.resize(static_cast<size_t>(settings.unique_mesh_count) * settings.subdivisions_count);
    for (uint32_t& mesh_subset_texture_index : mesh_subset_texture_indices)
    {
        mesh_subset_texture_index = textures_distribution(rng);
    }

    // Randomly generate parameters of each asteroid in array
    const float    orbit_radius = settings.orbit_radius_ratio * settings.scale;
    const float    disc_radius  = settings.disc_radius_ratio  * settings.scale;

    std::normal_distribution<float>         normal_distribution;
    std::uniform_int_distribution<uint32_t> mesh_distribution(0u, settings.unique_mesh_count - 1);
    std::uniform_int_distribution<uint32_t> colors_distribution(0, static_cast<uint32_t>(Asteroid::color_schema_size - 1));
    std::uniform_real_distribution<float>   scale_distribution(settings.min_asteroid_scale_ratio, settings.max_asteroid_scale_ratio);
    std::uniform_real_distribution<float>   scale_proportion_distribution(0.8f, 1.2f);
    std::uniform_real_distribution<float>   spin_velocity_distribution(-1.7f, 1.7f);
    std::uniform_real_distribution<float>   orbit_velocity_distribution(1.5f, 5.f);
    std::normal_distribution<float>         orbit_radius_distribution(orbit_radius, 0.6f * disc_radius);
    std::normal_distribution<float>         orbit_height_distribution(0.0f, 0.4f * disc_radius);

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
        cml::matrix_translation(translation_matrix, asteroid_orbit_radius, asteroid_orbit_height, 0.f);

        gfx::Matrix44f scale_matrix;
        cml::matrix_scale(scale_matrix, asteroid_scale_ratios * settings.scale);

        gfx::Matrix44f scale_translate_matrix = scale_matrix * translation_matrix;

        Asteroid::Colors asteroid_colors = normal_distribution(rng) <= 1.f
                                         ? Asteroid::GetAsteroidIceColors(colors_distribution(rng), colors_distribution(rng))
                                         : Asteroid::GetAsteroidRockColors(colors_distribution(rng), colors_distribution(rng));

        parameters.emplace_back(
            Asteroid::Parameters
            {
                asteroid_index,
                asteroid_mesh_index,
                settings.textures_array_enabled ? textures_distribution(rng) : 0u,
                std::move(asteroid_colors),
                std::move(scale_translate_matrix),
                GetRandomDirection(rng),
                asteroid_scale,
                orbit_velocity_distribution(rng) / (asteroid_scale * asteroid_orbit_radius),
                spin_velocity_distribution(rng)  / asteroid_scale,
                cml::constants<float>::pi() * normal_distribution(rng),
                cml::constants<float>::pi() * normal_distribution(rng) * 2.f
            }
        );
    }
}

AsteroidsArray::AsteroidsArray(gfx::RenderContext& context, Settings settings)
    : AsteroidsArray(context, settings, *std::make_shared<ContentState>(settings))
{
    ITT_FUNCTION_TASK();
}

AsteroidsArray::AsteroidsArray(gfx::RenderContext& context, Settings settings, ContentState& state)
    : BaseBuffers(context, state.uber_mesh, "Asteroids Array")
    , m_settings(std::move(settings))
    , m_sp_content_state(state.shared_from_this())
    , m_mesh_subset_by_instance_index(m_settings.instance_count, 0u)
    , m_min_mesh_lod_screen_size_log_2(std::log2(m_settings.mesh_lod_min_screen_size))
{
    ITT_FUNCTION_TASK();
    SCOPE_TIMER("AsteroidsArray::AsteroidsArray");
    
    const gfx::RenderContext::Settings& context_settings = context.GetSettings();

    const size_t textures_array_size = m_settings.textures_array_enabled ? m_settings.textures_count : 1;
    const gfx::Shader::MacroDefinitions macro_definitions  = { { "TEXTURES_COUNT", std::to_string(textures_array_size) } };

    gfx::RenderState::Settings state_settings;
    state_settings.sp_program = gfx::Program::Create(context,
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
                { { gfx::Shader::Type::Pixel,  "g_scene_uniforms" }, gfx::Program::Argument::Modifiers::Constant    },
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
    state_settings.sp_program->SetName("Asteroid Shaders");
    state_settings.viewports     = { gfx::GetFrameViewport(context_settings.frame_size) };
    state_settings.scissor_rects = { gfx::GetFrameScissorRect(context_settings.frame_size) };
    state_settings.depth.enabled = true;
    state_settings.depth.compare = m_settings.depth_reversed ? gfx::Compare::GreaterEqual : gfx::Compare::Less;
    
    m_sp_render_state = gfx::RenderState::Create(context, state_settings);
    m_sp_render_state->SetName("Asteroids Render State");

    SetInstanceCount(m_settings.instance_count);

    // Create texture arrays initialized with sub-resources data
    uint32_t texture_index = 0u;
    m_unique_textures.reserve(m_settings.textures_count);
    for(const gfx::Resource::SubResources& texture_subresources : m_sp_content_state->texture_array_subresources)
    {
        m_unique_textures.emplace_back(gfx::Texture::CreateImage(context, m_settings.texture_dimensions, static_cast<uint32_t>(texture_subresources.size()), gfx::PixelFormat::RGBA8Unorm, true));
        m_unique_textures.back()->SetData(texture_subresources);
        m_unique_textures.back()->SetName("Asteroid Texture " + std::to_string(texture_index++));
    }

    // Distribute textures between unique mesh subsets
    for (uint32_t subset_index = 0; subset_index < m_sp_content_state->mesh_subset_texture_indices.size(); ++subset_index)
    {
        const uint32_t subset_texture_index = m_sp_content_state->mesh_subset_texture_indices[subset_index];
        assert(subset_texture_index < m_unique_textures.size());
        SetSubsetTexture(m_unique_textures[subset_texture_index], subset_index);
    }
    
    m_sp_texture_sampler = gfx::Sampler::Create(context, {
        { gfx::Sampler::Filter::MinMag::Linear     },
        { gfx::Sampler::Address::Mode::ClampToZero }
    });
    m_sp_texture_sampler->SetName("Asteroid Texture Sampler");

    // Initialize default uniforms to be ready to render right aways
    Update(0.0, 0.0);
}
    
Ptrs<gfx::ProgramBindings> AsteroidsArray::CreateProgramBindings(const Ptr<gfx::Buffer> &sp_constants_buffer,
                                                                            const Ptr<gfx::Buffer> &sp_scene_uniforms_buffer,
                                                                            const Ptr<gfx::Buffer> &sp_asteroids_uniforms_buffer)
{
    ITT_FUNCTION_TASK();
    SCOPE_TIMER("AsteroidsArray::CreateProgramBindings");

    Ptrs<gfx::ProgramBindings> program_bindings_array;
    if (m_settings.instance_count == 0)
        return program_bindings_array;

    const gfx::Resource::Locations face_texture_locations = m_settings.textures_array_enabled
                                                          ? gfx::Resource::CreateLocations(m_unique_textures)
                                                          : gfx::Resource::Locations{ { GetInstanceTexturePtr(0) } };
    
    program_bindings_array.resize(m_settings.instance_count);
    program_bindings_array[0] = gfx::ProgramBindings::Create(m_sp_render_state->GetSettings().sp_program, {
        { { gfx::Shader::Type::All,    "g_mesh_uniforms"  }, { { sp_asteroids_uniforms_buffer, GetUniformsBufferOffset(0) } } },
        { { gfx::Shader::Type::Pixel,  "g_scene_uniforms" }, { { sp_scene_uniforms_buffer } } },
        { { gfx::Shader::Type::Pixel,  "g_constants"      }, { { sp_constants_buffer      } } },
        { { gfx::Shader::Type::Pixel,  "g_face_textures"  },     face_texture_locations       },
        { { gfx::Shader::Type::Pixel,  "g_texture_sampler"}, { { m_sp_texture_sampler     } } },
    });

    Data::ParallelFor<uint32_t>(1u, m_settings.instance_count, [&](uint32_t asteroid_index)
    {
        const Data::Size asteroid_uniform_offset = GetUniformsBufferOffset(asteroid_index);
        gfx::ProgramBindings::ResourceLocationsByArgument set_resource_location_by_argument = {
            { { gfx::Shader::Type::All, "g_mesh_uniforms"  }, { { sp_asteroids_uniforms_buffer, asteroid_uniform_offset } } },
        };
        if (!m_settings.textures_array_enabled)
        {
            set_resource_location_by_argument.insert(
                { { gfx::Shader::Type::Pixel, "g_face_textures"  }, { { GetInstanceTexturePtr(asteroid_index) } } }
            );
        }
        program_bindings_array[asteroid_index] = gfx::ProgramBindings::CreateCopy(*program_bindings_array[0], set_resource_location_by_argument);
    });
    
    return program_bindings_array;
}

void AsteroidsArray::Resize(const gfx::FrameSize &frame_size)
{
    ITT_FUNCTION_TASK();

    assert(m_sp_render_state);
    m_sp_render_state->SetViewports({ gfx::GetFrameViewport(frame_size) });
    m_sp_render_state->SetScissorRects({ gfx::GetFrameScissorRect(frame_size) });
}

bool AsteroidsArray::Update(double elapsed_seconds, double /*delta_seconds*/)
{
    ITT_FUNCTION_TASK();
    SCOPE_TIMER("AsteroidsArray::Update");

    gfx::Matrix44f view_matrix, proj_matrix;
    m_settings.view_camera.GetViewProjMatrices(view_matrix, proj_matrix);
    const gfx::Vector3f& eye_position = m_settings.view_camera.GetOrientation().eye;

    const gfx::Matrix44f view_proj_matrix = view_matrix * proj_matrix;
    const float elapsed_radians = cml::constants<float>::pi()* static_cast<float>(elapsed_seconds);

    Data::ParallelForEach<Parameters::iterator, Asteroid::Parameters>(m_sp_content_state->parameters.begin(), m_sp_content_state->parameters.end(),
        [this, &view_proj_matrix, elapsed_radians, &eye_position](Asteroid::Parameters& asteroid_parameters)
        {
            ITT_FUNCTION_TASK();

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
            const uint32_t          mesh_subdivision_index  = std::min(m_settings.subdivisions_count - 1, static_cast<uint32_t>(std::max(0.0f, mesh_subdiv_float)));
            const uint32_t          mesh_subset_index       = m_sp_content_state->uber_mesh.GetSubsetIndex(asteroid_parameters.mesh_instance_index, mesh_subdivision_index);
            const gfx::Vector2f&    mesh_subset_depth_range = m_sp_content_state->uber_mesh.GetSubsetDepthRange(mesh_subset_index);
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
        }
    );

    return true;
}

void AsteroidsArray::Draw(gfx::RenderCommandList &cmd_list, gfx::MeshBufferBindings& buffer_bindings)
{
    ITT_FUNCTION_TASK();
    SCOPE_TIMER("AsteroidsArray::Draw");

    const Data::Size uniforms_buffer_size = GetUniformsBufferSize();
    assert(buffer_bindings.sp_uniforms_buffer);
    assert(buffer_bindings.sp_uniforms_buffer->GetDataSize() >= uniforms_buffer_size);
    buffer_bindings.sp_uniforms_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&GetFinalPassUniforms()), uniforms_buffer_size } });

    cmd_list.Reset(m_sp_render_state, "Asteroids rendering");

    assert(buffer_bindings.program_bindings_per_instance.size() == m_settings.instance_count);
    BaseBuffers::Draw(cmd_list, buffer_bindings.program_bindings_per_instance,
                      gfx::ProgramBindings::ApplyBehavior::ConstantOnce);
}

void AsteroidsArray::DrawParallel(gfx::ParallelRenderCommandList& parallel_cmd_list, gfx::MeshBufferBindings& buffer_bindings)
{
    ITT_FUNCTION_TASK();
    SCOPE_TIMER("AsteroidsArray::DrawParallel");

    const Data::Size uniforms_buffer_size = GetUniformsBufferSize();
    assert(buffer_bindings.sp_uniforms_buffer);
    assert(buffer_bindings.sp_uniforms_buffer->GetDataSize() >= uniforms_buffer_size);
    buffer_bindings.sp_uniforms_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&GetFinalPassUniforms()), uniforms_buffer_size } });

    parallel_cmd_list.Reset(m_sp_render_state, "Asteroids Rendering");

    assert(buffer_bindings.program_bindings_per_instance.size() == m_settings.instance_count);
    BaseBuffers::DrawParallel(parallel_cmd_list, buffer_bindings.program_bindings_per_instance,
                              gfx::ProgramBindings::ApplyBehavior::ConstantOnce);
}

float AsteroidsArray::GetMinMeshLodScreenSize() const
{
    ITT_FUNCTION_TASK();
    return std::pow(2.f, m_min_mesh_lod_screen_size_log_2);
}

void AsteroidsArray::SetMinMeshLodScreenSize(float mesh_lod_min_screen_size)
{
    ITT_FUNCTION_TASK();
    m_min_mesh_lod_screen_size_log_2 = std::log2(mesh_lod_min_screen_size);
}

uint32_t AsteroidsArray::GetSubsetByInstanceIndex(uint32_t instance_index) const
{
    ITT_FUNCTION_TASK();

    assert(instance_index < m_mesh_subset_by_instance_index.size());
    return m_mesh_subset_by_instance_index[instance_index];
}

} // namespace Methane::Samples
