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

namespace data = Methane::Data;
namespace pal = Methane::Platform;
namespace pin = Methane::Platform::Input;

@implementation AppViewController
{
    pal::AppMac*     m_app_ptr;
    NSRect  m_frame_rect;
    bool        m_is_initialized;
    std::string m_error;
}

- (id) initWithApp : (pal::AppMac*) app_ptr andFrameRect : (NSRect) frame_rect
{
    META_FUNCTION_TASK();

    self = [super init];
    if (!self)
        return nil;
    
    m_app_ptr = app_ptr;
    m_frame_rect = frame_rect;
    m_is_initialized = false;
    
    return self;
}

-(NSWindow*) window
{
    META_FUNCTION_TASK();
    return m_app_ptr ? m_app_ptr->GetWindow() : nil;
}

- (pal::AppMac*) getApp
{
    return m_app_ptr;
}

- (void) loadView
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);
    m_app_ptr->InitContextWithErrorHandling({ self }, data::FrameSize(m_frame_rect.size.width, m_frame_rect.size.height));
}

- (void)viewDidLoad
{
    META_FUNCTION_TASK();
    [super viewDidLoad];
    [self.view.window makeFirstResponder:self];
}

- (void)appView: (nonnull AppViewMetal *) view drawableSizeWillChange: (CGSize)size
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);

    if (!m_is_initialized)
    {
        m_is_initialized = m_app_ptr->InitWithErrorHandling();
    }
    m_frame_rect = [view frame];
    m_app_ptr->Resize( data::FrameSize(size.width, size.height), false);
}

- (void) drawInView: (nonnull AppViewMetal*) view
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);
    #pragma unused(view)

    if (!m_is_initialized)
    {
        m_is_initialized = m_app_ptr->InitWithErrorHandling();
    }
    m_app_ptr->UpdateAndRenderWithErrorHandling();
}

// ====== Keyboard event handlers ======

- (void) keyDown:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);

    m_app_ptr->ProcessInputWithErrorHandling(&pin::IActionController::OnKeyboardChanged,
        pin::Keyboard::KeyConverter({ [event keyCode], [event modifierFlags] }).GetKey(), pin::Keyboard::KeyState::Pressed);
}

- (void) keyUp:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);

    m_app_ptr->ProcessInputWithErrorHandling(&pin::IActionController::OnKeyboardChanged,
        pin::Keyboard::KeyConverter({ [event keyCode], [event modifierFlags] }).GetKey(), pin::Keyboard::KeyState::Released);
}

- (void) flagsChanged:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);

    m_app_ptr->ProcessInputWithErrorHandling(&pin::IActionController::OnModifiersChanged, pin::Keyboard::KeyConverter({ [event keyCode], [event modifierFlags] }).GetModifiers());
}

// ====== Mouse event handlers ======

- (void)mouseMoved:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);

    NSPoint pos = [event locationInWindow];
    pos.x *= self.view.window.backingScaleFactor;
    pos.y = (m_frame_rect.size.height - pos.y) * self.view.window.backingScaleFactor;
    m_app_ptr->ProcessInputWithErrorHandling(&pin::IActionController::OnMousePositionChanged,
                                           pin::Mouse::Position{ static_cast<int>(pos.x), static_cast<int>(pos.y) });
}

- (void)mouseDown:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);
    #pragma unused(event)

    m_app_ptr->ProcessInputWithErrorHandling(&pin::IActionController::OnMouseButtonChanged,
                                           pin::Mouse::Button::Left, pin::Mouse::ButtonState::Pressed);
}

- (void)mouseUp:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);
    #pragma unused(event)

    m_app_ptr->ProcessInputWithErrorHandling(&pin::IActionController::OnMouseButtonChanged,
                                           pin::Mouse::Button::Left, pin::Mouse::ButtonState::Released);
}

- (void)mouseDragged:(NSEvent *)event
{
    META_FUNCTION_TASK();
    [self mouseMoved:event];
}

- (void)rightMouseDown:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);
    #pragma unused(event)

    m_app_ptr->ProcessInputWithErrorHandling(&pin::IActionController::OnMouseButtonChanged,
                                           pin::Mouse::Button::Right, pin::Mouse::ButtonState::Pressed);
}

- (void)rightMouseUp:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);
    #pragma unused(event)

    m_app_ptr->ProcessInputWithErrorHandling(&pin::IActionController::OnMouseButtonChanged,
                                           pin::Mouse::Button::Right, pin::Mouse::ButtonState::Released);
}

- (void)rightMouseDragged:(NSEvent *)event
{
    META_FUNCTION_TASK();
    [self mouseMoved:event];
}

- (void)otherMouseDown:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);

    m_app_ptr->ProcessInputWithErrorHandling(&pin::IActionController::OnMouseButtonChanged,
                                           static_cast<pin::Mouse::Button>(static_cast<int>([event buttonNumber])), pin::Mouse::ButtonState::Pressed);
}

- (void)otherMouseUp:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);

    m_app_ptr->ProcessInputWithErrorHandling(&pin::IActionController::OnMouseButtonChanged,
                                           static_cast<pin::Mouse::Button>(static_cast<int>([event buttonNumber])), pin::Mouse::ButtonState::Released);
}

- (void)otherMouseDragged:(NSEvent *)event
{
    META_FUNCTION_TASK();
    [self mouseMoved:event];
}

- (void)mouseEntered:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);
    #pragma unused(event)

    m_app_ptr->ProcessInputWithErrorHandling(&pin::IActionController::OnMouseInWindowChanged, true);
}

- (void)mouseExited:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);
    #pragma unused(event)

    m_app_ptr->ProcessInputWithErrorHandling(&pin::IActionController::OnMouseInWindowChanged, false);
}

- (void)scrollWheel:(NSEvent *)event
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_app_ptr);

    pin::Mouse::Scroll scroll([event scrollingDeltaX], -[event scrollingDeltaY]);
    if ([event hasPreciseScrollingDeltas])
        scroll *= 0.1F;
    
    if (fabs(scroll.GetX()) < 0.00001 && fabs(scroll.GetY()) > 0.00001)
        return;

    m_app_ptr->ProcessInputWithErrorHandling(&pin::IActionController::OnMouseScrollChanged, scroll);
}

@end
