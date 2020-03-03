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

FILE: Methane/Platform/MacOS/AppViewMT.hh
MacOS application view implementation.

******************************************************************************/

#pragma once

#import "AppEnvironment.hh"

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@interface AppViewMT : NSView
{
    IBOutlet NSViewController* viewController;
}

@property (nonatomic, readonly, nullable) NSWindow* appWindow;
@property (nonatomic, readonly, nullable) NSScreen* currentScreen;
@property (nonatomic, readonly) MTLPixelFormat pixelFormat;
@property (nonatomic, readwrite) NSUInteger drawableCount;
@property (nonatomic, readwrite) BOOL vsyncEnabled;

@property (nonatomic, readwrite) NSTimeInterval unsyncRefreshInterval;
@property (nonatomic, readwrite) BOOL redrawing;

@property (nonatomic, weak, nullable) id<MetalAppViewDelegate> delegate;
@property (nonatomic, readonly, nullable) CAMetalLayer* metalLayer;
@property (nonatomic, readonly, nullable) id<CAMetalDrawable> currentDrawable;

- (nonnull instancetype)initWithCoder:(nonnull NSCoder*) aDecoder;
- (nonnull instancetype)initWithFrame:(NSRect) backing_frame
                            appWindow:(nullable NSWindow*) app_window
                               device:(nonnull id<MTLDevice>) device
                          pixelFormat:(MTLPixelFormat) pixelFormat
                        drawableCount:(NSUInteger) drawable_count
                         vsyncEnabled:(BOOL) vsync_enabled
                unsyncRefreshInterval:(double) refresh_interval_sec;

@end
