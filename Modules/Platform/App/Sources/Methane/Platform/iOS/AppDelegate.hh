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

FILE: Methane/Platform/iOS/AppDelegate.h
iOS application delegate implementation.

******************************************************************************/

#include "AppViewController.hh"

#include <Methane/Platform/AppBase.h>

#import <UIKit/UIKit.h>

namespace Methane::Platform { class AppMac; }

@interface AppDelegate : UIResponder<UIApplicationDelegate>

@property (nonatomic, strong, nonnull) IBOutlet AppViewController* viewController;
@property (nonatomic, readwrite, nullable, retain) UIWindow* window;

- (id _Nullable) init;
- (void) run;
- (void) alert : (nonnull NSString*) ns_title withInformation: (nonnull NSString*) ns_info andStyle: (UIAlertActionStyle) ns_alert_style;

@end
