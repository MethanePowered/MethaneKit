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

FILE: Methane/Platform/MacOS/AppDelegate.mm
MacOS application delegate implementation.

******************************************************************************/

#import "AppDelegate.hh"

#ifdef APPLE_MACOS
#import "WindowDelegate.hh"
#endif

#include <Methane/Platform/MacOS/AppMac.hh>
#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

using namespace Methane;
using namespace Methane::Platform;

@implementation AppDelegate
{
#ifdef APPLE_MACOS
    WindowDelegate* m_window_delegate;
#endif
}

@synthesize window = m_window;

- (id) initWithApp : (AppMac*) p_app andSettings : (const AppBase::Settings*) p_settings
{
    META_FUNCTION_TASK();

    self = [super init];
    if (!self || !p_settings)
        return nil;

    const Methane::Data::FloatSize& frame_size = p_settings->size;

    NativeScreen* ns_main_screen = [NativeScreen mainScreen];
#ifdef APPLE_MACOS
    const auto& ns_frame_size = ns_main_screen.frame.size;
#else
    const auto& ns_frame_size = ns_main_screen.bounds.size;
#endif

    CGFloat frame_width = frame_size.GetWidth() < 1.0
                          ? ns_frame_size.width * (frame_size.GetWidth() > 0.0 ? frame_size.GetWidth() : 0.7)
                          : static_cast<CGFloat>(frame_size.GetWidth());
    CGFloat frame_height = frame_size.GetHeight() < 1.0
                           ? ns_frame_size.height * (frame_size.GetHeight() > 0.0 ? frame_size.GetHeight() : 0.7)
                           : static_cast<CGFloat>(frame_size.GetHeight());

#ifdef APPLE_MACOS
    const Methane::Data::FrameSize& min_frame_size = p_settings->min_size;
    NSRect frame = NSMakeRect(0, 0, frame_width, frame_height);

    NSUInteger style_mask = NSWindowStyleMaskTitled |
                            NSWindowStyleMaskResizable |
                            NSWindowStyleMaskClosable |
                            NSWindowStyleMaskMiniaturizable;

    NSBackingStoreType backing = NSBackingStoreBuffered;

    m_window = [[NSWindow alloc] initWithContentRect:frame styleMask:style_mask backing:backing defer:YES];
    m_window.contentMinSize = NSMakeSize(static_cast<CGFloat>(min_frame_size.GetWidth()), static_cast<CGFloat>(min_frame_size.GetHeight()));
    m_window.title    = MacOS::ConvertToNsType<std::string, NSString*>(p_settings->name);

    m_window_delegate = [[WindowDelegate alloc] initWithApp:p_app];
    m_window.delegate = m_window_delegate;

    [m_window center];
    
    NSRect backing_frame = [ns_main_screen convertRectToBacking:frame];
#else
    const CGRect backing_frame = CGRectMake(0, 0,
                                            frame_width * ns_main_screen.nativeScale,
                                            frame_height * ns_main_screen.nativeScale);
#endif

    self.viewController = [[AppViewController alloc] initWithApp:p_app andFrameRect:backing_frame];
    
    p_app->SetWindow(m_window);
    
    return self;
}

- (void) run
{
    META_FUNCTION_TASK();
#ifdef APPLE_MACOS
    [m_window setContentViewController: self.viewController];
    [m_window setAcceptsMouseMovedEvents:YES];
#endif
}

- (void) alert : (NSString*) ns_title withInformation: (NSString*) ns_info andStyle: (NativeAlertStyle) ns_alert_style
{
    META_FUNCTION_TASK();
#ifdef APPLE_MACOS
    META_CHECK_ARG_NOT_NULL(ns_title);
    META_CHECK_ARG_NOT_NULL(ns_info);
    
    NSAlert *ns_alert = [[NSAlert alloc] init];
    [ns_alert addButtonWithTitle:ns_alert_style == NSAlertStyleCritical ? @"Exit" : @"OK"];
    [ns_alert setMessageText:ns_title];
    [ns_alert setInformativeText:ns_info];
    [ns_alert setAlertStyle:ns_alert_style];
    [ns_alert runModal];
    
    if (ns_alert.alertStyle == NSAlertStyleCritical)
    {
        [NSApp terminate:self];
    }
#else
    // TODO: iOS implementation missing
#endif
}

#ifdef APPLE_MACOS

- (void) applicationWillFinishLaunching:(NSNotification *)notification
{
    META_FUNCTION_TASK();
    #pragma unused(notification)
    [m_window makeKeyAndOrderFront:self];
}

- (void) applicationDidFinishLaunching:(NSNotification *)notification
{
    META_FUNCTION_TASK();
    #pragma unused(notification)
    [m_window makeFirstResponder: self.viewController.view];
    [m_window makeKeyAndOrderFront: nil];
}

- (void) windowWillEnterFullScreen:(NSNotification *)notification
{
    META_FUNCTION_TASK();
    #pragma unused(notification)
    AppMac* p_app = [self.viewController getApp];
    META_CHECK_ARG_NOT_NULL(p_app);
    p_app->SetFullScreenInternal(true);
}

- (void) windowWillExitFullScreen:(NSNotification *)notification
{
    META_FUNCTION_TASK();
    #pragma unused(notification)
    AppMac* p_app = [self.viewController getApp];
    META_CHECK_ARG_NOT_NULL(p_app);
    p_app->SetFullScreenInternal(false);
}

- (void) applicationWillTerminate:(NSNotification *)notification
{
    META_FUNCTION_TASK();
    #pragma unused(notification)
    // Insert code here to tear down your application
}

- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    META_FUNCTION_TASK();
    #pragma unused(sender)
    return YES;
}

#endif // APPLE_MACOS

@end
