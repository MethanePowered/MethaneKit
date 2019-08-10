/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Platform/MacOS/AppDelegate.mm
MacOS application delegate implementation.

******************************************************************************/

#import "AppDelegate.h"

#include <Methane/Platform/MacOS/AppMac.h>
#include <Methane/Platform/MacOS/Types.h>

#include <cassert>

using namespace Methane;
using namespace Methane::Platform;

@implementation AppDelegate

@synthesize window = _window;

- (id) initWithApp : (AppMac*) p_app andSettings : (AppBase::Settings*) p_settings
{
    self = [super init];
    if (!self || !p_settings)
        return nil;

    NSScreen* mainScreen = [NSScreen mainScreen];
    CGFloat frame_width = p_settings->width < 1.0 ? mainScreen.frame.size.width * (p_settings->width > 0.0 ? p_settings->width : 0.7)
                                                  : static_cast<CGFloat>(p_settings->width);
    CGFloat frame_height = p_settings->height < 1.0 ? mainScreen.frame.size.height * (p_settings->height > 0.0 ? p_settings->height : 0.7)
                                                  : static_cast<CGFloat>(p_settings->height);
    NSRect frame = NSMakeRect(0, 0, frame_width, frame_height);

    NSUInteger styleMask =  NSWindowStyleMaskTitled |
                            NSWindowStyleMaskResizable |
                            NSWindowStyleMaskClosable |
                            NSWindowStyleMaskMiniaturizable;

    NSBackingStoreType backing = NSBackingStoreBuffered;

    _window = [[NSWindow alloc] initWithContentRect:frame styleMask:styleMask backing:backing defer:YES];
    _window.title = MacOS::ConvertToNSType<std::string, NSString*>(p_settings->name);
    [_window center];
    
    NSRect backing_frame = [mainScreen convertRectToBacking:frame];
    self.viewController = [[AppViewController alloc] initWithApp:p_app andFrameRect:backing_frame];
    
    p_app->SetWindow(_window);
    
    return self;
}

- (void) run
{
    [self.window setContentViewController: self.viewController];
    [self.window setAcceptsMouseMovedEvents:YES];
}

- (void) alert : (NSString*) ns_title withInformation: (NSString*) ns_info andStyle: (NSAlertStyle) ns_alert_style
{
    if (ns_title == nil || ns_info == nil)
    {
        assert(0);
        return;
    }
    
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
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification
{
    [self.window makeKeyAndOrderFront:self];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    [self.window makeFirstResponder: self.viewController.view];
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    // Insert code here to tear down your application
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

@end
