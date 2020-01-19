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

FILE: Methane/Platform/MacOS/AppEnvironment.h
MacOS application environment.

******************************************************************************/

#pragma once

#ifdef __OBJC__

#import <AppKit/AppKit.h>

@class AppViewMT;

@protocol MetalAppViewDelegate <NSObject>

@property (nonatomic, readonly, nullable) NSWindow* window;

- (void)drawInView:(nonnull AppViewMT *)view;
- (void)appView: (nonnull AppViewMT *) view drawableSizeWillChange: (CGSize)size;

@end

using NativeViewController = NSViewController<MetalAppViewDelegate>;

#else

using NativeViewController = void;

#endif

namespace Methane::Platform
{

struct AppEnvironment
{
    NativeViewController* _Nonnull ns_app_delegate;
};

} // namespace Methane::Platform
