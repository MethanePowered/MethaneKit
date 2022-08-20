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
}

@synthesize window = m_window;

- (id) init
{
    META_FUNCTION_TASK();
    self = [super init];
    if (!self)
        return nil;

    AppMac* p_app = AppMac::GetInstance();
    UIScreen* ns_main_screen = [UIScreen mainScreen];
    const auto& ns_frame_size = ns_main_screen.bounds.size;
    const CGRect backing_frame = CGRectMake(0.f, 0.f,
                                            ns_frame_size.width * ns_main_screen.nativeScale,
                                            ns_frame_size.height * ns_main_screen.nativeScale);
    
    m_window = [[UIWindow alloc] initWithFrame:ns_main_screen.bounds];
    
    self.viewController = [[AppViewController alloc] initWithApp:p_app andFrameRect:backing_frame];
    
    UINavigationController* navigation_controller = [[UINavigationController alloc] initWithRootViewController:self.viewController];
    navigation_controller.navigationBarHidden = YES;
    self.window.rootViewController = navigation_controller;

    p_app->SetWindow(m_window);
    return self;
}

- (void) run
{
    META_FUNCTION_TASK();
}

- (void) alert : (NSString*) ns_title withInformation: (NSString*) ns_info andStyle: (UIAlertActionStyle) ns_alert_style
{
    META_FUNCTION_TASK();
    META_FUNCTION_NOT_IMPLEMENTED_DESCR("iOS alert is not implemented");
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id> *)launch_otions
{
    META_FUNCTION_TASK();
    #pragma unused(launch_otions)
    [self.window makeKeyAndVisible];
    return YES;
}

@end
