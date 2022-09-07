/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Platform/iOS/AppViewController.mm
iOS application view controller implementation.

******************************************************************************/

#import "AppViewController.hh"

#include <Methane/Platform/iOS/AppIOS.hh>
#include <Methane/Data/Types.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <string>

using namespace Methane;
using namespace Methane::Platform;

namespace Methane::Platform
{

static Mouse::Button GetMouseButtonByTouchesCount(uint32_t touches_cout)
{
    META_FUNCTION_TASK();
    switch(touches_cout)
    {
    case 1U: return Mouse::Button::Left;
    case 2U: return Mouse::Button::Right;
    case 3U: return Mouse::Button::Middle;
    default: META_UNEXPECTED_ARG_DESCR(touches_cout, "Methane iOS application supports from 1 to 3 touches handling only");
    }
}

} // namespace Methane::Platform

@implementation AppViewController
{
    AppIOS*     m_p_app;
    CGRect      m_frame_rect;
    bool        m_is_initialized;
    std::string m_error;
}

- (id)initWithApp : (Methane::Platform::AppIOS*) p_app andFrameRect : (CGRect) frame_rect
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

-(nullable UIWindow*) window
{
    META_FUNCTION_TASK();
    return m_p_app ? m_p_app->GetWindow() : nil;
}

- (Methane::Platform::AppIOS*) getApp
{
    return m_p_app;
}

- (void) loadView
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    m_p_app->InitContextWithErrorHandling({ self }, Data::FrameSize(m_frame_rect.size.width, m_frame_rect.size.height));
}

- (void)viewDidLoad
{
    META_FUNCTION_TASK();
    [super viewDidLoad];
#ifdef APPLE_IOS
    self.view.multipleTouchEnabled = YES;
#endif
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
    m_p_app->Resize( Data::FrameSize(size.width, size.height), false);
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

- (void)touchesBegan:(NSSet<UITouch *> *)touches
           withEvent:(UIEvent *)event
{
    META_FUNCTION_TASK();
    #pragma unused(event)

    if (touches.count < 1 || touches.count > 3)
    {
        [super touchesBegan:touches withEvent:event];
        return;
    }

    [self handleTouchPosition:[touches anyObject]];
    [self handeTouches:touches withMouseButtonChange:Mouse::ButtonState::Pressed];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches
           withEvent:(UIEvent *)event
{
    META_FUNCTION_TASK();
    #pragma unused(event)

    if (touches.count < 1 || touches.count > 3)
    {
        [super touchesEnded:touches withEvent:event];
        return;
    }

    [self handleTouchPosition:[touches anyObject]];
    [self handeTouches:touches withMouseButtonChange:Mouse::ButtonState::Released];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches
               withEvent:(UIEvent *)event
{
    META_FUNCTION_TASK();
    #pragma unused(event)

    if (touches.count < 1 || touches.count > 3)
    {
        [super touchesCancelled:touches withEvent:event];
        return;
    }

    [self handeTouches:touches withMouseButtonChange:Mouse::ButtonState::Released];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches
           withEvent:(UIEvent *)event
{
    META_FUNCTION_TASK();
    #pragma unused(event)

    if (touches.count < 1 || touches.count > 3)
    {
        [super touchesMoved:touches withEvent:event];
        return;
    }

    [self handleTouchPosition:[touches anyObject]];
}

- (void)handleTouchPosition:(UITouch *) touch
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);

    CGPoint pos = [touch locationInView:self.view];
    UIScreen* ns_main_screen = [UIScreen mainScreen];
    pos.x *= ns_main_screen.nativeScale;
    pos.y *= ns_main_screen.nativeScale;

    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnMousePositionChanged, Mouse::Position{ static_cast<int>(pos.x), static_cast<int>(pos.y) });
}

- (void) handeTouches:(NSSet<UITouch *> *)touches withMouseButtonChange:(Mouse::ButtonState) mouse_button_state
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    const Mouse::Button mouse_button = GetMouseButtonByTouchesCount(static_cast<uint32_t>(touches.count));
    m_p_app->ProcessInputWithErrorHandling(&Input::IActionController::OnMouseButtonChanged, mouse_button, mouse_button_state);
}

@end
