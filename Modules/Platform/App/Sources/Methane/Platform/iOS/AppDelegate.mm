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

FILE: Methane/Platform/iOS/AppDelegate.mm
iOS application delegate implementation.

******************************************************************************/

#import "AppDelegate.hh"

#include <Methane/Platform/iOS/AppIOS.hh>
#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

using namespace Methane;
using namespace Methane::Platform;

@implementation AppDelegate
{
    UIWindow* m_window;
}

@synthesize window = m_window;

- (id) init
{
    META_FUNCTION_TASK();
    self = [super init];
    if (!self)
        return nil;
    
    AppIOS*   app_ptr          = AppIOS::GetInstance();
    UIScreen* ns_main_screen   = [UIScreen mainScreen];
    const auto& ns_frame_size  = ns_main_screen.bounds.size;
    const CGRect backing_frame = CGRectMake(0.f, 0.f,
                                            ns_frame_size.width * ns_main_screen.nativeScale,
                                            ns_frame_size.height * ns_main_screen.nativeScale);
    self.viewController = [[AppViewController alloc] initWithApp:app_ptr andFrameRect:backing_frame];
    
    m_window = nil;
    return self;
}

- (void) run
{ }

- (void) alert : (NSString*) ns_title withInformation: (NSString*) ns_info andStyle: (UIAlertActionStyle) ns_alert_style
{
    META_FUNCTION_TASK();
    
    UIAlertController* alert = [UIAlertController
                                    alertControllerWithTitle: ns_title
                                    message                 : ns_info
                                    preferredStyle          : UIAlertControllerStyleAlert];

    UIAlertAction* ok_button = [UIAlertAction
                                    actionWithTitle : ns_alert_style == UIAlertActionStyleDestructive ? @"Exit" : @"Ok"
                                    style           : ns_alert_style
                                    handler         : ^(UIAlertAction*)
                                    {
                                        if (ns_alert_style != UIAlertActionStyleDestructive)
                                            return;
        
                                        // Suspend and then terminate the application
                                        [[UIApplication sharedApplication] performSelector:@selector(suspend)];
                                        [NSThread sleepForTimeInterval:2.0];
                                        exit(0);
                                    }];
    
    [alert addAction:ok_button];

    [self.viewController presentViewController:alert animated:YES completion:nil];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id> *)launch_otions
{
    META_FUNCTION_TASK();
    #pragma unused(launch_otions)
    [self.window makeKeyAndVisible];
    return YES;
}

- (UIWindow*) window
{
    META_FUNCTION_TASK();
    if (!m_window)
    {
        UIScreen* ns_main_screen = [UIScreen mainScreen];
        m_window = [[UIWindow alloc] initWithFrame:ns_main_screen.bounds];
        
        UINavigationController* navigation_controller = [[UINavigationController alloc] initWithRootViewController:self.viewController];
        navigation_controller.navigationBarHidden = YES;
        m_window.rootViewController = navigation_controller;
        
        AppIOS::GetInstance()->SetWindow(m_window);
    }
    return m_window;
}

@end
