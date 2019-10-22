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

FILE: Asteroid.cpp
Random generated asteroid model with mesh and texture ready for rendering

******************************************************************************/

#include "Asteroid.h"

namespace Methane::Samples
{

Asteroid::Mesh::Mesh()
    : gfx::IcosahedronMesh<Vertex>(VertexLayoutFromArray(Vertex::layout), 0.5f, 0, false)
{
}

Asteroid::Asteroid(gfx::Context& context)
    : BaseBuffers(context, Mesh(), "Asteroid")
{
}

} // namespace Methane::Samples
