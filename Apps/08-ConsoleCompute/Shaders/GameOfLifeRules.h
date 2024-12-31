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

FILE: MethaneKit/Apps/Tutorials/08-ConsoleCompute/Shaders/GameOfLifeRules.h
Game of Life Rules constants

******************************************************************************/

#ifndef METHANE_KIT_GAMEOFLIFERULES_H
#define METHANE_KIT_GAMEOFLIFERULES_H

#ifdef __cplusplus
#include <vector>
#include <string>

// Game of Life alternative rules:
// https://conwaylife.com/wiki/List_of_Life-like_rules
// NOTE: indices of rules should match constants in "Shaders/GameOfLifeRules.h"
static const std::vector<std::string> g_gol_rule_labels{
    "Classic (B3/S23)",
    "Star Trek (B3/S0248)",
    "Coral (B3/S45678)",
    "Geology (B3578/S24678)",
    "Vote (B5678/S45678)"
};

using uint = uint32_t;
using uint3 = std::array<uint, 3>;

#endif

struct Constants
{
    uint   game_rule_id;
    uint3 _padding;
};

#ifndef __cplusplus

static const uint g_game_rule_classic    = 0; // B3/S23
static const uint g_game_rule_flock      = 1; // B3/S12
static const uint g_game_rule_star_trek  = 2; // B3/S0248
static const uint g_game_rule_coral      = 3; // B3/S45678
static const uint g_game_rule_geology    = 4; // B3578/S24678
static const uint g_game_rule_vote       = 5; // B5678/S45678

static bool IsCellSurvived(uint game_rule_id, uint alive_neighbors_count)
{
    switch(game_rule_id)
    {
    case g_game_rule_classic:
        return alive_neighbors_count == 2 ||
               alive_neighbors_count == 3;

    case g_game_rule_flock:
        return alive_neighbors_count == 1 ||
               alive_neighbors_count == 2;

    case g_game_rule_star_trek:
        return alive_neighbors_count == 0 ||
               alive_neighbors_count == 2 ||
               alive_neighbors_count == 4 ||
               alive_neighbors_count == 8;

    case g_game_rule_coral:
    case g_game_rule_vote:
        return alive_neighbors_count >= 4 &&
               alive_neighbors_count <= 8;

    case g_game_rule_geology:
        return alive_neighbors_count == 2 ||
               alive_neighbors_count == 4 ||
               alive_neighbors_count == 6 ||
               alive_neighbors_count == 7 ||
               alive_neighbors_count == 8;
    }
    return 0;
}

static bool IsCellBorn(uint game_rule_id, uint alive_neighbors_count)
{
    switch(game_rule_id)
    {
    case g_game_rule_classic:
    case g_game_rule_flock:
    case g_game_rule_star_trek:
    case g_game_rule_coral:
        return alive_neighbors_count == 3;

    case g_game_rule_geology:
        return alive_neighbors_count == 3 ||
               alive_neighbors_count == 5 ||
               alive_neighbors_count == 7 ||
               alive_neighbors_count == 8;

    case g_game_rule_vote:
        return alive_neighbors_count >= 5 &&
               alive_neighbors_count <= 8;
    }
    return 0;
}
#endif

#endif //METHANE_KIT_GAMEOFLIFERULES_H
