/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Tests/UserInterface/Types/FakePlatformApp.hpp
Fake platform application implementation for UI types testing

******************************************************************************/

#include <Methane/Platform/IApp.h>
#include <Methane/Checks.hpp>

namespace Methane::Platform
{

struct AppEnvironment;

class FakeApp : public IApp
{
public:
    FakeApp(float content_scale, uint32_t font_dpi)
        : m_content_scale(content_scale)
        , m_font_dpi(font_dpi)
    { }

    // IApp implementation
    int      Run(const RunArgs&) override                                           { META_FUNCTION_NOT_IMPLEMENTED(); }
    void     InitContext(const AppEnvironment&, const Data::FrameSize&) override    { META_FUNCTION_NOT_IMPLEMENTED(); }
    void     Init() override                                                        { META_FUNCTION_NOT_IMPLEMENTED(); }
    void     ChangeWindowBounds(const Data::FrameRect&) override                    { META_FUNCTION_NOT_IMPLEMENTED(); }
    void     StartResizing() override                                               { META_FUNCTION_NOT_IMPLEMENTED(); }
    void     EndResizing() override                                                 { META_FUNCTION_NOT_IMPLEMENTED(); }
    bool     Resize(const Data::FrameSize&, bool) override                          { META_FUNCTION_NOT_IMPLEMENTED(); }
    bool     Update() override                                                      { META_FUNCTION_NOT_IMPLEMENTED(); }
    bool     Render() override                                                      { META_FUNCTION_NOT_IMPLEMENTED(); }
    void     Alert(const Message&, bool) override                                   { META_FUNCTION_NOT_IMPLEMENTED(); }
    void     SetWindowTitle(const std::string&) override                            { META_FUNCTION_NOT_IMPLEMENTED(); }
    bool     SetFullScreen(bool) override                                           { META_FUNCTION_NOT_IMPLEMENTED(); }
    bool     SetKeyboardFocus(bool) override                                        { META_FUNCTION_NOT_IMPLEMENTED(); }
    void     ShowControlsHelp() override                                            { META_FUNCTION_NOT_IMPLEMENTED(); }
    void     ShowCommandLineHelp() override                                         { META_FUNCTION_NOT_IMPLEMENTED(); }
    void     ShowParameters() override                                              { META_FUNCTION_NOT_IMPLEMENTED(); }
    float    GetContentScalingFactor() const override                               { return m_content_scale; }
    uint32_t GetFontResolutionDpi() const override                                  { return m_font_dpi; }
    void     Close() override                                                       { META_FUNCTION_NOT_IMPLEMENTED(); }

private:
    const float    m_content_scale;
    const uint32_t m_font_dpi;
};

} // namespace Methane::Platform

