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

FILE: Methane/Graphics/UberMesh.hpp
Uber mesh generator with customizable vertex type

******************************************************************************/

#pragma once

#include "BaseMesh.hpp"

namespace Methane::Graphics
{

template<typename VType>
class UberMesh : public BaseMesh<VType>
{
public:
    using BaseMeshT = BaseMesh<VType>;

    explicit UberMesh(const Mesh::VertexLayout& vertex_layout)
        : BaseMeshT(Mesh::Type::Uber, vertex_layout)
    {
        ITT_FUNCTION_TASK();
    }

    void AddSubMesh(const BaseMeshT& sub_mesh, bool adjust_indices)
    {
        ITT_FUNCTION_TASK();
        const typename BaseMeshT::Vertices& sub_vertices = sub_mesh.GetVertices();
        const Mesh::Indices& sub_indices = sub_mesh.GetIndices();

        m_subsets.emplace_back(sub_mesh.GetType(),
                               Mesh::Subset::Slice(static_cast<Data::Size>(BaseMeshT::m_vertices.size()), static_cast<Data::Size>(sub_vertices.size())),
                               Mesh::Subset::Slice(static_cast<Data::Size>(Mesh::m_indices.size()), static_cast<Data::Size>(sub_indices.size())),
                               adjust_indices);

        if (adjust_indices)
        {
            const Data::Size vertex_count = BaseMeshT::GetVertexCount();
            assert(vertex_count <= std::numeric_limits<Mesh::Index>::max());

            const Mesh::Index index_offset = static_cast<Mesh::Index>(vertex_count);
            std::transform(sub_indices.begin(), sub_indices.end(), std::back_inserter(Mesh::m_indices),
                           [index_offset](const Mesh::Index& index)
                               {
                                   assert(static_cast<Data::Size>(index_offset) + index <= std::numeric_limits<Mesh::Index>::max());
                                   return index_offset + index;
                               });
        }
        else
        {
            Mesh::m_indices.insert(Mesh::m_indices.end(), sub_indices.begin(), sub_indices.end());
        }

        BaseMeshT::m_vertices.insert(BaseMeshT::m_vertices.end(), sub_vertices.begin(), sub_vertices.end());
    }

    const Mesh::Subsets& GetSubsets() const                     { return m_subsets; }
    size_t               GetSubsetCount() const noexcept        { return m_subsets.size(); }
    const Mesh::Subset&  GetSubset(size_t subset_index) const
    {
        ITT_FUNCTION_TASK();
        if (subset_index >= m_subsets.size())
            throw std::invalid_argument("Sub mesh index is out of bounds.");

        return m_subsets[subset_index];
    }

    std::pair<const VType*, size_t> GetSubsetVertices(size_t subset_index) const
    {
        ITT_FUNCTION_TASK();
        const Mesh::Subset& subset = GetSubset(subset_index);
        return { BaseMeshT::GetVertices().data() + subset.vertices.offset, subset.vertices.count };
    }

    std::pair<const Mesh::Index*, size_t> GetSubsetIndices(size_t subset_index) const
    {
        ITT_FUNCTION_TASK();
        const Mesh::Subset& subset = GetSubset(subset_index);
        return { Mesh::GetIndices().data() + subset.indices.offset, subset.indices.count };
    }

private:
    Mesh::Subsets m_subsets;
};

} // namespace Methane::Graphics
