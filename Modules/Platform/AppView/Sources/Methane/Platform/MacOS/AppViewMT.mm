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

FILE: Methane/Platform/MacOS/AppViewMT.mm
MacOS application view implementation.

******************************************************************************/

#import <Methane/Platform/MacOS/AppViewMT.hh>

#include <Methane/Instrumentation.h>
#include <Methane/TracyGpu.hpp>
#include <Methane/Data/TimeRange.hpp>

#include <set>
#include <mutex>

#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED

class DrawablePresentScope;
using DrawablePresentScopeSet = std::set<DrawablePresentScope>;

class DrawablePresentScope
{
public:
    DrawablePresentScope(id<CAMetalDrawable> drawable, TRACY_GPU_SCOPE_TYPE&& present_scope,
                         DrawablePresentScopeSet& scope_set, std::mutex& scope_set_mutex)
        : m_drawable(drawable)
        , m_present_scope(std::move(present_scope))
    {
        if (@available(macOS 10.15.4, *))
        {
            [drawable addPresentedHandler:^(id<MTLDrawable> mtl_drawable) {
                META_LOG("Metal Drawable (" + std::to_string(mtl_drawable.drawableID) + ") was presented at " + std::to_string(mtl_drawable.presentedTime) + " sec.");
                const Methane::Data::Timestamp presented_timestamp = Methane::Data::ConvertTimeSecondsToNanoseconds(mtl_drawable.presentedTime);
                TRACY_GPU_SCOPE_COMPLETE(m_present_scope, Methane::Data::TimeRange(presented_timestamp, presented_timestamp));
                std::lock_guard<std::mutex> lock(scope_set_mutex);
                scope_set.erase(*this);
            }];
        }
    }

    bool operator<(const DrawablePresentScope& other) const noexcept
    {
        return m_drawable < other.m_drawable;
    }

private:
    id<CAMetalDrawable> __strong m_drawable;
    TRACY_GPU_SCOPE_TYPE         m_present_scope;
};

#endif

@interface AppViewMT ()
{
    NSTrackingArea* m_tracking_area;
    id<CAMetalDrawable> __strong m_current_drawable;
    std::unique_ptr<Methane::Tracy::GpuContext> m_sp_gpu_context;

#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    DrawablePresentScopeSet m_present_scopes;
    std::mutex              m_present_scopes_mutex;
#endif
}

@property (nonatomic) id <MTLDevice> device;
@property (nonatomic) NSTimer* unsyncTimer;

@end

@implementation AppViewMT

@synthesize device = m_device;
@synthesize unsyncTimer = m_unsync_timer;
@synthesize appWindow = m_app_window;
@synthesize pixelFormat = m_pixel_format;
@synthesize drawableCount = m_drawable_count;
@synthesize vsyncEnabled = m_vsync_enabled;
@synthesize unsyncRefreshInterval = m_unsync_refresh_interval;
@synthesize redrawing = m_redrawing;

CVDisplayLinkRef g_display_link;

static CVReturn OnDisplayLinkFrame(CVDisplayLinkRef /*display_link*/,
                                   const CVTimeStamp* /*now*/,
                                   const CVTimeStamp* /*output_time*/,
                                   CVOptionFlags /*flags_in*/,
                                   CVOptionFlags* /*flags_out*/,
                                   void* p_display_link_context)
{
    META_FUNCTION_TASK();

    AppViewMT* app_view = (__bridge AppViewMT*) p_display_link_context;
    
    if (app_view.redrawing)
    {
        // Rendering in Main thread
        [app_view performSelectorOnMainThread:@selector(redraw) withObject:nil waitUntilDone:YES];
    }

    return kCVReturnSuccess;
}

- (instancetype) initWithCoder:(NSCoder *)aDecoder
{
    META_FUNCTION_TASK();

    if ((self = [super initWithCoder:aDecoder]))
    {
        m_app_window = nil;
        m_device = MTLCreateSystemDefaultDevice();
        m_pixel_format = MTLPixelFormatBGRA8Unorm;
        m_drawable_count = 3;
        m_vsync_enabled = YES;
        m_unsync_refresh_interval = 1.0 / 120;
        m_current_drawable = nil;
        m_sp_gpu_context = std::make_unique<Methane::Tracy::GpuContext>(
            Methane::Tracy::GpuContext::Settings(
                Methane::Data::ConvertTimeSecondsToNanoseconds(CACurrentMediaTime())
            )
        );
        [self commonInit];
    }
    
    return self;
}

- (instancetype) initWithFrame:(NSRect)backing_frame
                     appWindow:(NSWindow*) app_window
                        device:(id<MTLDevice>) device
                   pixelFormat:(MTLPixelFormat) pixel_format
                 drawableCount:(NSUInteger) drawable_count
                  vsyncEnabled:(BOOL) vsync_enabled
         unsyncRefreshInterval:(double) refresh_interval_sec
{
    META_FUNCTION_TASK();

    NSScreen* current_screen = (app_window != nil) ? [app_window screen] : [NSScreen mainScreen];
    NSRect frame = [current_screen convertRectFromBacking:backing_frame];
    if ((self = [super initWithFrame:NSRectToCGRect(frame)]))
    {
        m_app_window = app_window;
        m_device = device;
        m_pixel_format = pixel_format;
        m_drawable_count = drawable_count;
        m_vsync_enabled = vsync_enabled;
        m_unsync_refresh_interval = refresh_interval_sec;
        m_current_drawable = nil;
        m_sp_gpu_context = std::make_unique<Methane::Tracy::GpuContext>(
            Methane::Tracy::GpuContext::Settings(
                Methane::Data::ConvertTimeSecondsToNanoseconds(CACurrentMediaTime())
            )
        );
        [self updateTrackingAreas];
        [self commonInit];
    }
    
    return self;
}

- (void) dealloc
{
    META_FUNCTION_TASK();

    self.redrawing = NO;
    [super dealloc];
}

- (void) commonInit
{
    META_FUNCTION_TASK();

    self.metalLayer.pixelFormat = self.pixelFormat;
    self.wantsLayer = YES;
    self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawOnSetNeedsDisplay;
    
    NSNotificationCenter* ns_notification_center = [NSNotificationCenter defaultCenter];
    [ns_notification_center addObserver:self
                           selector:@selector(windowWillClose:)
                               name:NSWindowWillCloseNotification
                             object:self.window];
}

- (NSScreen*) currentScreen
{
    META_FUNCTION_TASK();
    return self.appWindow != nil ? [self.appWindow screen] : [NSScreen mainScreen];
}

- (CAMetalLayer*) metalLayer
{
    META_FUNCTION_TASK();
    return (CAMetalLayer*) self.layer;
}

- (CALayer *) makeBackingLayer
{
    META_FUNCTION_TASK();

    CAMetalLayer *layer = [[CAMetalLayer alloc] init];
    layer.bounds = self.bounds;
    layer.device = self.device;
    layer.pixelFormat = self.pixelFormat;
    layer.displaySyncEnabled = self.vsyncEnabled;
    layer.contentsScale = self.currentScreen.backingScaleFactor;

    if (@available(macOS 10.13.2, *))
    {
        layer.maximumDrawableCount = self.drawableCount;
    }
    
    return layer;
}

- (void) layout
{
    META_FUNCTION_TASK();

    [super layout];
    CGFloat scale = self.window ? self.window.screen.backingScaleFactor
                                : [NSScreen mainScreen].backingScaleFactor;

    CGSize drawable_size = self.bounds.size;
    drawable_size.width *= scale;
    drawable_size.height *= scale;

    self.metalLayer.drawableSize = drawable_size;
    m_current_drawable = nil;
    
    if ([self.delegate respondsToSelector:@selector(appView:drawableSizeWillChange:)])
    {
        [self.delegate appView: self drawableSizeWillChange: drawable_size];
    }
}

- (id<CAMetalDrawable>) currentDrawable
{
    META_FUNCTION_TASK();
    if (m_current_drawable)
        return m_current_drawable;

#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    TRACY_GPU_SCOPE_TYPE gpu_scope(TRACY_GPU_SCOPE_INIT(*m_sp_gpu_context));
    TRACY_GPU_SCOPE_BEGIN(gpu_scope, "Request/Present Metal Drawable");
    m_current_drawable = [self.metalLayer nextDrawable];
    TRACY_GPU_SCOPE_END(gpu_scope);
    std::lock_guard<std::mutex> lock(m_present_scopes_mutex);
    m_present_scopes.emplace(m_current_drawable, std::move(gpu_scope), m_present_scopes, m_present_scopes_mutex);
#else
    m_current_drawable = [self.metalLayer nextDrawable];
#endif

    return m_current_drawable;
}

- (void) setUnsyncRefreshInterval: (NSTimeInterval) unsync_refresh_interval
{
    META_FUNCTION_TASK();
    if (m_unsync_refresh_interval == unsync_refresh_interval)
        return;
    
    m_unsync_refresh_interval = unsync_refresh_interval;
    
    if (self.redrawing && !self.vsyncEnabled)
    {
        [self.unsyncTimer invalidate];
        self.unsyncTimer = [NSTimer scheduledTimerWithTimeInterval:m_unsync_refresh_interval target:self selector:@selector(redraw) userInfo:nil repeats:YES];
    }
}

- (void) setVsyncEnabled: (BOOL) vsync_enabled
{
    META_FUNCTION_TASK();
    if (m_vsync_enabled == vsync_enabled)
        return;
    
    BOOL was_redrawing = self.redrawing;
    self.redrawing = NO;
    m_vsync_enabled = vsync_enabled;
    self.metalLayer.displaySyncEnabled = vsync_enabled;
    self.redrawing = was_redrawing;
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
    
    // Stop non-sync refresh timer if it's running
    if (self.unsyncTimer)
    {
        [self.unsyncTimer invalidate];
        self.unsyncTimer = nil;
    }

    // Enable/Disable redrawing on VSync
    if (m_redrawing == YES)
    {
        if (m_vsync_enabled)
        {
            CVReturn cv_return = CVDisplayLinkCreateWithActiveCGDisplays(&g_display_link);
            assert(cv_return == kCVReturnSuccess);

            cv_return = CVDisplayLinkSetOutputCallback(g_display_link, &OnDisplayLinkFrame, (__bridge void *)self);
            assert(cv_return == kCVReturnSuccess);

            cv_return = CVDisplayLinkSetCurrentCGDisplay(g_display_link, CGMainDisplayID());
            assert(cv_return == kCVReturnSuccess);
            
            CVDisplayLinkStart(g_display_link);
        }
        else
        {
            // Create non-sync refresh timer
            self.unsyncTimer = [NSTimer scheduledTimerWithTimeInterval:m_unsync_refresh_interval target:self selector:@selector(redraw) userInfo:nil repeats:YES];
        }
    }
    else if (g_display_link)
    {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
        CVDisplayLinkStop(g_display_link);
        CVDisplayLinkRelease(g_display_link);
        g_display_link = nil;
    }
}

- (void) redraw
{
    META_FUNCTION_TASK();
    if (!self.redrawing)
        return;
    
    bool delegate_can_draw_in_view = [self.delegate respondsToSelector:@selector(drawInView:)];
    assert(delegate_can_draw_in_view);
    if (!delegate_can_draw_in_view)
        return;

    m_current_drawable = nil;
    [self.delegate drawInView:self];
    m_current_drawable = nil;
}

- (void) windowWillClose:(NSNotification*)notification
{
    META_FUNCTION_TASK();

    // Stop the display link when the window is closing because we will
    // not be able to get a drawable, but the display link may continue to fire
    if (notification.object == self.window && g_display_link)
    {
        self.redrawing = NO;
    }
}

- (void)setViewController:(NSViewController *)newController
{
    META_FUNCTION_TASK();
    if (viewController)
    {
        NSResponder* controller_next_responder = [viewController nextResponder];
        [super setNextResponder:controller_next_responder];
        [viewController setNextResponder:nil];
    }
    
    viewController = newController;
    
    if (newController)
    {
        NSResponder* own_next_responder = [self nextResponder];
        [super setNextResponder: viewController];
        [viewController setNextResponder:own_next_responder];
    }
}

- (void)updateTrackingAreas
{
    META_FUNCTION_TASK();
    if (m_tracking_area != nil)
    {
        [self removeTrackingArea:m_tracking_area];
        [m_tracking_area release];
    }
    
    const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
                                          NSTrackingActiveInKeyWindow |
                                          NSTrackingEnabledDuringMouseDrag |
                                          NSTrackingCursorUpdate |
                                          NSTrackingInVisibleRect |
                                          NSTrackingAssumeInside;
    
    m_tracking_area = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                options:options
                                                  owner:self
                                               userInfo:nil];
    
    [self addTrackingArea:m_tracking_area];
    [super updateTrackingAreas];
}

- (void)setNextResponder:(NSResponder *)newNextResponder
{
    META_FUNCTION_TASK();
    if (viewController)
    {
        [viewController setNextResponder:newNextResponder];
        return;
    }
    [super setNextResponder:newNextResponder];
}

- (BOOL)acceptsFirstResponder
{
    META_FUNCTION_TASK();
    return YES;
}

- (BOOL)canBecomeKeyView
{
    META_FUNCTION_TASK();
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event
{
    META_FUNCTION_TASK();
    #pragma unused(event)
    return YES;
}

@end
