/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Platform/MacOS/AppDelegate.h
MacOS application delegate implementation.

******************************************************************************/

#include "AppViewController.hh"

#include <Methane/Platform/AppBase.h>

#import <Cocoa/Cocoa.h>

namespace Methane { namespace Platform { class AppMac; } }

@interface AppDelegate : NSObject<NSApplicationDelegate>

@property (nonatomic, strong, nonnull) IBOutlet AppViewController* viewController;
@property (nonatomic, readonly, nullable) NSWindow* window;

- (id _Nullable) initWithApp : (Methane::Platform::AppMac* _Nonnull) p_app andSettings : (const Methane::Platform::AppBase::Settings* _Nonnull) p_settings;
- (void) run;
- (void) alert : (nonnull NSString*) ns_title withInformation: (nonnull NSString*) ns_info andStyle: (NSAlertStyle) ns_alert_style;

@end
