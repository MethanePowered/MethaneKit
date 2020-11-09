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

FILE: Methane/Platform/MacOS/AppViewController.mm
MacOS application view controller implementation.

******************************************************************************/

#import "AppViewController.hh"

#include <Methane/Platform/MacOS/AppMac.hh>
#include <Methane/Data/Types.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <string>

using namespace Methane;
using namespace Methane::Platform;

@implementation AppViewController
{
    AppMac*     m_p_app;
    NSRect      m_frame_rect;
    bool        m_is_initialized;
    std::string m_error;
}

- (id) initWithApp : (Methane::Platform::AppMac*) p_app andFrameRect : (NSRect) frame_rect
{
    META_FUNCTION_TASK();

    self = [super init];
    if (!self)
        return nil;
    
    m_p_app = p_app;
    m_frame_rect = frame_rect;
    m_is_initialized = false;
    
    return self;
}

-(NSWindow*) window
{
    META_FUNCTION_TASK();
    return m_p_app ? m_p_app->GetWindow() : nil;
}

- (Methane::Platform::AppMac*) getApp
{
    return m_p_app;
}

- (void) loadView
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    m_p_app->InitContextWithErrorHandling({ self }, { static_cast<uint32_t>(m_frame_rect.size.width), static_cast<uint32_t>(m_frame_rect.size.height) });
}

- (void)viewDidLoad
{
    META_FUNCTION_TASK();
    [super viewDidLoad];
    [self.view.window makeFirstResponder:self];
}

- (void)appView: (nonnull AppViewMT *) view drawableSizeWillChange: (CGSize)size
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);

    if (!m_is_initialized)
    {
        m_is_initialized = m_p_app->InitWithErrorHandling();
    }
    m_frame_rect = [view frame];
    m_p_app->Resize( { static_cast<uint32_t>(size.width), static_cast<uint32_t>(size.height) }, false );
}

- (void) drawInView: (nonnull AppViewMT*) view
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(view)

    if (!m_is_initialized)
    {
        m_is_initialized = m_p_app->InitWithErrorHandling();
    }
    m_p_app->UpdateAndRenderWithErrorHandling();
}

// ====== Keyboard event handlers ======

- (void) keyDown:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);

    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnKeyboardChanged, Keyboard::KeyConverter({ [event keyCode], [event modifierFlags] }).GetKey(), Keyboard::KeyState::Pressed);
}

- (void) keyUp:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);

    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnKeyboardChanged, Keyboard::KeyConverter({ [event keyCode], [event modifierFlags] }).GetKey(), Keyboard::KeyState::Released);
}

- (void) flagsChanged:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);

    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnModifiersChanged, Keyboard::KeyConverter({ [event keyCode], [event modifierFlags] }).GetModifiers());
}

// ====== Mouse event handlers ======

- (void)mouseMoved:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);

    NSPoint pos = [event locationInWindow];
    pos.x *= self.view.window.backingScaleFactor;
    pos.y = (m_frame_rect.size.height - pos.y) * self.view.window.backingScaleFactor;
    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnMousePositionChanged, Mouse::Position{ static_cast<int>(pos.x), static_cast<int>(pos.y) });
}

- (void)mouseDown:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(event)

    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnMouseButtonChanged, Mouse::Button::Left, Mouse::ButtonState::Pressed);
}

- (void)mouseUp:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(event)

    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnMouseButtonChanged, Mouse::Button::Left, Mouse::ButtonState::Released);
}

- (void)mouseDragged:(NSEvent *)event
{
    META_FUNCTION_TASK();
    [self mouseMoved:event];
}

- (void)rightMouseDown:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(event)

    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnMouseButtonChanged, Mouse::Button::Right, Mouse::ButtonState::Pressed);
}

- (void)rightMouseUp:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(event)

    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnMouseButtonChanged, Mouse::Button::Right, Mouse::ButtonState::Released);
}

- (void)rightMouseDragged:(NSEvent *)event
{
    META_FUNCTION_TASK();
    [self mouseMoved:event];
}

- (void)otherMouseDown:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);

    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnMouseButtonChanged, static_cast<Mouse::Button>(static_cast<int>([event buttonNumber])), Mouse::ButtonState::Pressed);
}

- (void)otherMouseUp:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);

    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnMouseButtonChanged, static_cast<Mouse::Button>(static_cast<int>([event buttonNumber])), Mouse::ButtonState::Released);
}

- (void)otherMouseDragged:(NSEvent *)event
{
    META_FUNCTION_TASK();
    [self mouseMoved:event];
}

- (void)mouseEntered:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(event)

    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnMouseInWindowChanged, true);
}

- (void)mouseExited:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(event)

    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnMouseInWindowChanged, false);
}

- (void)scrollWheel:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);

    Mouse::Scroll scroll([event scrollingDeltaX], -[event scrollingDeltaY]);
    if ([event hasPreciseScrollingDeltas])
        scroll *= 0.1F;
    
    if (fabs(scroll.GetX()) < 0.00001 && fabs(scroll.GetY()) > 0.00001)
        return;

    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnMouseScrollChanged, scroll);
}

@end
