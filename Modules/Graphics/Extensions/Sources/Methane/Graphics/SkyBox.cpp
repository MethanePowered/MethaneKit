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

FILE: SkyBox.cpp
SkyBox rendering primitive

******************************************************************************/

#include <Methane/Graphics/SkyBox.h>
#include <Methane/Graphics/Mesh.h>

namespace Methane::Graphics
{

struct SkyBoxVertex
{
    Mesh::Position position;

    using FieldsArray = std::array<Mesh::VertexField, 1>;
    static constexpr const FieldsArray layout = {
        Mesh::VertexField::Position,
    };
};

SkyBox::SkyBox(Context& context, ImageLoader& image_loader, const Settings& settings)
    : m_settings(settings)
    , m_context(context)
    , m_mesh_buffers(context, BoxMesh<SkyBoxVertex>(Mesh::VertexLayoutFromArray(SkyBoxVertex::layout)), "Sky-Box")
{
}

} // namespace Methane::Graphics