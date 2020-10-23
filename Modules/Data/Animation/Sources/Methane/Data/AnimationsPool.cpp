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
    META_FUNCTION_TASK();
    if (empty())
        return;

    if (m_is_paused)
    {
        if (m_is_dry_update_on_pause_enabled)
            DryUpdate();
        return;
    }

    std::vector<size_t> completed_animation_indices;
    for (size_t animation_index = 0; animation_index < size(); ++animation_index)
    {
        Ptr<Animation>& animation_ptr = (*this)[animation_index];
        if (!animation_ptr || !animation_ptr->Update())
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

void AnimationsPool::DryUpdate()
{
    META_FUNCTION_TASK();
    for(const Ptr<Animation>& animation_ptr : *this)
    {
        if (!animation_ptr)
            continue;

        animation_ptr->DryUpdate();
    }
}

void AnimationsPool::Pause()
{
    META_FUNCTION_TASK();
    if (m_is_paused)
        return;

    for(const Ptr<Animation>& animation_ptr : *this)
    {
        if (animation_ptr->GetState() == Animation::State::Running)
            animation_ptr->Pause();
    }

    m_is_paused = true;
}

void AnimationsPool::Resume()
{
    META_FUNCTION_TASK();
    if (!m_is_paused)
        return;

    for(const Ptr<Animation>& animation_ptr : *this)
    {
        if (animation_ptr->GetState() == Animation::State::Paused)
            animation_ptr->Resume();
    }

    m_is_paused = false;
}

} // namespace Methane::Data
