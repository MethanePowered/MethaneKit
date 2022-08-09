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

FILE: Methane/Platform/MacOS/AppDelegate.h
MacOS application delegate implementation.

******************************************************************************/

#include "AppViewController.hh"

#include <Methane/Platform/AppBase.h>

#ifdef APPLE_MACOS
#import <Cocoa/Cocoa.h>
using NativeScreen = NSScreen;
using NativeAppDelegate = NSObject<NSApplicationDelegate>;
using NativeAlertStyle = NSAlertStyle;
#else
#import <UIKit/UIKit.h>
using NativeScreen = UIScreen;
using NativeAppDelegate = UIResponder<UIApplicationDelegate>;
using NativeAlertStyle = UIAlertActionStyle;
#endif

namespace Methane::Platform { class AppMac; }

@interface AppDelegate : NativeAppDelegate

@property (nonatomic, strong, nonnull) IBOutlet AppViewController* viewController;
@property (nonatomic, readwrite, nullable, retain) NativeWindow* window;

- (id _Nullable) init;
- (id _Nullable) initWithApp : (Methane::Platform::AppMac* _Nonnull) p_app andSettings : (const Methane::Platform::AppBase::Settings* _Nonnull) p_settings;
- (void) run;
- (void) alert : (nonnull NSString*) ns_title withInformation: (nonnull NSString*) ns_info andStyle: (NativeAlertStyle) ns_alert_style;

@end
