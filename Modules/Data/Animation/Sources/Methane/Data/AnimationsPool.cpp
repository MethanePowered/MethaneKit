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

FILE: Methane/Data/AnimationsPool.cpp
Pool of animations for centralized updating, adding and removing in application.

******************************************************************************/

#include <Methane/Data/AnimationsPool.h>
#include <Methane/Instrumentation.h>

#include <vector>

namespace Methane::Data
{

void AnimationsPool::Update()
{
    ITT_FUNCTION_TASK();
    if (m_is_paused || empty())
        return;

    std::vector<size_t> completed_animation_indices;
    for (size_t animation_index = 0; animation_index < size(); ++animation_index)
    {
        Ptr<Animation>& sp_animation = (*this)[animation_index];
        if (!sp_animation || !sp_animation->Update())
        {
            completed_animation_indices.push_back(animation_index);
        }
    }

    for (auto animation_index_it = completed_animation_indices.rbegin();
              animation_index_it != completed_animation_indices.rend(); 
            ++animation_index_it)
    {
        erase(begin() + *animation_index_it);
    }
}

void AnimationsPool::Pause()
{
    if (m_is_paused)
        return;

    for(const Ptr<Animation>& sp_animation : *this)
    {
        if (sp_animation->GetState() == Animation::State::Running)
            sp_animation->Pause();
    }

    m_is_paused = true;
}

void AnimationsPool::Resume()
{
    if (!m_is_paused)
        return;

    for(const Ptr<Animation>& sp_animation : *this)
    {
        if (sp_animation->GetState() == Animation::State::Paused)
            sp_animation->Resume();
    }

    m_is_paused = false;
}

} // namespace Methane::Data
