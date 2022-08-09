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

FILE: Methane/Platform/iOS/AppViewMT.mm
iOS application view implementation for Metal rendering.

******************************************************************************/

#import <Methane/Platform/MacOS/AppViewMT.hh>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

@interface AppViewMT ()
{
    CAMetalLayer*                m_metal_layer;
    __strong id<CAMetalDrawable> m_current_drawable;
    CADisplayLink*               m_display_link;
}
@end

@implementation AppViewMT

@synthesize appWindow = m_app_window;
@synthesize pixelFormat = m_pixel_format;
@synthesize drawableCount = m_drawable_count;
@synthesize vsyncEnabled = m_vsync_enabled;
@synthesize unsyncRefreshInterval = m_unsync_refresh_interval;
@synthesize redrawing = m_redrawing;

+ (Class) layerClass
{
    return [CAMetalLayer class];
}

- (instancetype) initWithCoder:(NSCoder *)aDecoder
{
    META_FUNCTION_TASK();
    if ((self = [super initWithCoder:aDecoder]))
    {
        m_app_window = nil;
        m_pixel_format = MTLPixelFormatBGRA8Unorm;
        m_drawable_count = 3;
        m_vsync_enabled = YES;
        m_current_drawable = nil;
        [self commonInit];
    }
    return self;
}

- (instancetype) initWithFrame:(CGRect)frame
                     appWindow:(UIWindow*) app_window
                   pixelFormat:(MTLPixelFormat) pixel_format
                 drawableCount:(NSUInteger) drawable_count
                  vsyncEnabled:(BOOL) vsync_enabled
         unsyncRefreshInterval:(double) refresh_interval_sec
{
    META_FUNCTION_TASK();
    if (self = [super initWithFrame:frame])
    {
        m_app_window = app_window;
        m_pixel_format = pixel_format;
        m_drawable_count = drawable_count;
        m_vsync_enabled = vsync_enabled;
        m_unsync_refresh_interval = refresh_interval_sec;
        m_current_drawable = nil;
        [self commonInit];
    }
    return self;
}

- (void) dealloc
{
    META_FUNCTION_TASK();
    [m_display_link invalidate];
}

- (void) commonInit
{
    META_FUNCTION_TASK();
    m_metal_layer = static_cast<CAMetalLayer*>(self.layer);
    self.layer.delegate = self;
    self.metalLayer.pixelFormat = self.pixelFormat;
}

- (UIScreen*) currentScreen
{
    META_FUNCTION_TASK();
    return self.appWindow != nil ? [self.appWindow screen] : [UIScreen mainScreen];
}

- (CAMetalLayer*) metalLayer
{
    META_FUNCTION_TASK();
    return (CAMetalLayer*) self.layer;
}

- (id<CAMetalDrawable>) currentDrawable
{
    META_FUNCTION_TASK();
    if (m_current_drawable)
        return m_current_drawable;

    m_current_drawable = [self.metalLayer nextDrawable];
    return m_current_drawable;
}

- (void) resizeDrawable
{
    META_FUNCTION_TASK();
    if (![self.delegate respondsToSelector:@selector(appView:drawableSizeWillChange:)])
        return;

    CGFloat scale = self.window ? self.window.screen.nativeScale : [UIScreen mainScreen].nativeScale;

    CGSize drawable_size = self.bounds.size;
    drawable_size.width  *= scale;
    drawable_size.height *= scale;

    if(drawable_size.width == self.metalLayer.drawableSize.width &&
        drawable_size.height == self.metalLayer.drawableSize.height)
        return;

    @autoreleasepool
    {
        self.metalLayer.drawableSize = drawable_size;
        m_current_drawable = nil;

        [self.delegate appView: self drawableSizeWillChange: drawable_size];
    }
}

- (void)setContentScaleFactor:(CGFloat)contentScaleFactor
{
    META_FUNCTION_TASK();
    [super setContentScaleFactor:contentScaleFactor];
    [self resizeDrawable];
}

- (void)layoutSubviews
{
    META_FUNCTION_TASK();
    [super layoutSubviews];
    [self resizeDrawable];
}

- (void)setFrame:(CGRect)frame
{
    META_FUNCTION_TASK();
    [super setFrame:frame];
    [self resizeDrawable];
}

- (void)setBounds:(CGRect)bounds
{
    META_FUNCTION_TASK();
    [super setBounds:bounds];
    [self resizeDrawable];
}

- (void)didMoveToWindow
{
    META_FUNCTION_TASK();
    [super didMoveToWindow];

    if(self.window == nil)
    {
        // If moving off of a window destroy the display link.
        [m_display_link invalidate];
        m_display_link = nil;
        return;
    }

    [self setupCADisplayLinkForScreen:self.appWindow.screen];

    // CADisplayLink callbacks are associated with an 'NSRunLoop'. The currentRunLoop is the
    // the main run loop (since 'didMoveToWindow' is always executed from the main thread.
    [m_display_link addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];

    [self resizeDrawable];
}

- (void) setVsyncEnabled: (BOOL) vsync_enabled
{
    META_FUNCTION_TASK();
    if (m_vsync_enabled == vsync_enabled)
        return;

    m_vsync_enabled = vsync_enabled;
}

- (void)setDrawableCount:(NSUInteger) drawable_count
{
    META_FUNCTION_TASK();
    if (m_drawable_count == drawable_count)
        return;

    BOOL was_redrawing = self.redrawing;
    self.redrawing = NO;
    m_drawable_count = drawable_count;
    if (@available(macOS 10.13.2, *))
    {
        self.metalLayer.maximumDrawableCount = self.drawableCount;
    }
    self.redrawing = was_redrawing;
}

- (void) setRedrawing: (BOOL) redrawing
{
    META_FUNCTION_TASK();
    if (m_redrawing == redrawing)
        return;

    m_redrawing = redrawing;
    m_display_link.paused = !redrawing;
}

- (void)setupCADisplayLinkForScreen:(UIScreen*)screen
{
    META_FUNCTION_TASK();
    [m_display_link invalidate];
    m_display_link = [screen displayLinkWithTarget:self selector:@selector(render)];
    m_display_link.paused = !self.redrawing;
}

- (void)didEnterBackground:(NSNotification*)notification
{
    META_FUNCTION_TASK();
    self.redrawing = NO;
}

- (void)willEnterForeground:(NSNotification*)notification
{
    META_FUNCTION_TASK();
    self.redrawing = YES;
}

- (void) render
{
    META_FUNCTION_TASK();
    if (!self.redrawing)
        return;
    
    bool delegate_can_draw_in_view = [self.delegate respondsToSelector:@selector(drawInView:)];
    META_CHECK_ARG_TRUE_DESCR(delegate_can_draw_in_view, "application delegate can not draw in view");

    @autoreleasepool
    {
        m_current_drawable = nil;
        [self.delegate drawInView:self];
        m_current_drawable = nil;
    }
}

@end
