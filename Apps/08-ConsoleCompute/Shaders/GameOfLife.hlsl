/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: MethaneKit/Apps/Tutorials/08-ConsoleCompute/Shaders/GameOfLife.hlsl
Compute shader for Conway's Game of Life

******************************************************************************/

#include "GameOfLifeRules.h"

[[vk::push_constant]]
ConstantBuffer<Constants> g_constants     : register(b0, META_ARG_CONSTANT);
RWTexture2D<uint>         g_frame_texture : register(u0, META_ARG_MUTABLE);

[numthreads(16, 16, 1)]
void MainCS(uint3 id : SV_DispatchThreadID)
{
    uint2 frame_texture_size;
    g_frame_texture.GetDimensions(frame_texture_size.x, frame_texture_size.y);
    if (id.x > frame_texture_size.x || id.y > frame_texture_size.y)
        return;

    // For a cell at id.xy compute number live neighbours,
    // which are 8 cells that are horizontally, vertically, or diagonally adjacent.
    uint alive_neighbors_count = 0;
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            if (x == 0 && y == 0)
                continue;

            uint2 neighbor_pos = uint2(id.x + x, id.y + y);
            if (g_frame_texture[neighbor_pos.xy] > 0)
                alive_neighbors_count++;
        }
    }

    g_frame_texture[id.xy] = ( g_frame_texture[id.xy] > 0
                               ? IsCellSurvived(g_constants.game_rule_id, alive_neighbors_count)
                               : IsCellBorn(g_constants.game_rule_id, alive_neighbors_count)
                             ) ? 1 : 0;
}
