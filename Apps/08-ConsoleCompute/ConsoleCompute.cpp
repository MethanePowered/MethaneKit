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

FILE: ConsoleCompute.cpp
Tutorial demonstrating "game of life" computing on GPU in console application

******************************************************************************/

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

int main(int, const char*[])
{
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    auto middle = ftxui::Renderer([] { return ftxui::text("middle") | ftxui::center; });
    auto left   = ftxui::Renderer([] { return ftxui::text("Left")   | ftxui::center; });
    auto right  = ftxui::Renderer([] { return ftxui::text("right")  | ftxui::center; });
    auto top    = ftxui::Renderer([] { return ftxui::text("top")    | ftxui::center; });
    auto bottom = ftxui::Renderer([] { return ftxui::text("bottom") | ftxui::center; });

    int left_size   = 8;
    int right_size  = 8;
    int top_size    = 5;
    int bottom_size = 5;

    auto container = middle;
    container = ftxui::ResizableSplitLeft(left, container, &left_size);
    container = ftxui::ResizableSplitRight(right, container, &right_size);
    container = ftxui::ResizableSplitTop(top, container, &top_size);
    container = ftxui::ResizableSplitBottom(bottom, container, &bottom_size);

    auto renderer = ftxui::Renderer(container, [&] { return container->Render() | ftxui::border; });
    screen.Loop(renderer);

    return 0;
}