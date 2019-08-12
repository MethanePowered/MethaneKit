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

FILE: Methane/Platform/MacOS/AppViewMT.mm
MacOS application view implementation.

******************************************************************************/

#import <Methane/Platform/MacOS/AppViewMT.h>

@interface AppViewMT ()
{
    NSTrackingArea* trackingArea;
}

@property (strong) id<CAMetalDrawable> currentDrawable;
@property (nonatomic) id <MTLDevice> device;
@property (nonatomic) NSTimer* unsyncTimer;

@end

@implementation AppViewMT

@synthesize appWindow = _appWindow;
@synthesize pixelFormat = _pixelFormat;
@synthesize drawableCount = _drawableCount;
@synthesize vsyncEnabled = _vsyncEnabled;
@synthesize unsyncRefreshInterval = _unsyncRefreshInterval;

CVDisplayLinkRef _displayLink;

static CVReturn OnDisplayLinkFrame(CVDisplayLinkRef displayLink,
                                   const CVTimeStamp *now,
                                   const CVTimeStamp *outputTime,
                                   CVOptionFlags flagsIn,
                                   CVOptionFlags *flagsOut,
                                   void *displayLinkContext)
{
    AppViewMT* app_view = (__bridge AppViewMT*)displayLinkContext;
    
    if (app_view.redrawing)
    {
        // Rendering in Main thread
        [app_view performSelectorOnMainThread:@selector(redraw) withObject:nil waitUntilDone:YES];
    }

    return kCVReturnSuccess;
}

- (instancetype) initWithCoder:(NSCoder *)aDecoder
{
    if ((self = [super initWithCoder:aDecoder]))
    {
        _appWindow = nil;
        _device = MTLCreateSystemDefaultDevice();
        _pixelFormat = MTLPixelFormatBGRA8Unorm;
        _drawableCount = 3;
        _vsyncEnabled = YES;
        _unsyncRefreshInterval = 1.0 / 120;
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
         unsyncRefreshInterval:(double) refresn_interval_sec
{
    NSScreen* current_screen = (app_window != nil) ? [app_window screen] : [NSScreen mainScreen];
    NSRect frame = [current_screen convertRectFromBacking:backing_frame];
    if ((self = [super initWithFrame:NSRectToCGRect(frame)]))
    {
        _appWindow = app_window;
        _device = device;
        _pixelFormat = pixel_format;
        _drawableCount = drawable_count;
        _vsyncEnabled = vsync_enabled;
        _unsyncRefreshInterval = refresn_interval_sec;
        [self updateTrackingAreas];
        [self commonInit];
    }
    
    return self;
}

- (void) dealloc
{
    self.redrawing = NO;
    [super dealloc];
}

- (void) commonInit
{
    self.metalLayer.pixelFormat = self.pixelFormat;
    self.wantsLayer = YES;
    self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawOnSetNeedsDisplay;
    
    NSNotificationCenter* notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter addObserver:self
                           selector:@selector(windowWillClose:)
                               name:NSWindowWillCloseNotification
                             object:self.window];
}

- (NSScreen*) currentScreen
{
    return self.appWindow != nil ? [self.appWindow screen] : [NSScreen mainScreen];
}

- (CAMetalLayer*) metalLayer
{
    return (CAMetalLayer*) self.layer;
}

- (CALayer *) makeBackingLayer
{
    CAMetalLayer *layer = [[CAMetalLayer alloc] init];
    layer.bounds = self.bounds;
    layer.device = self.device;
    layer.pixelFormat = self.pixelFormat;
    layer.maximumDrawableCount = self.drawableCount;
    layer.displaySyncEnabled = self.vsyncEnabled;
    layer.contentsScale = self.currentScreen.backingScaleFactor;
    
    return layer;
}

- (void) layout
{
    [super layout];
    CGFloat scale = self.window ? self.window.screen.backingScaleFactor
                                : [NSScreen mainScreen].backingScaleFactor;

    CGSize drawable_size = self.bounds.size;
    drawable_size.width *= scale;
    drawable_size.height *= scale;

    self.metalLayer.drawableSize = drawable_size;
    self.currentDrawable = [self.metalLayer nextDrawable];
    
    if ([self.delegate respondsToSelector:@selector(appView:drawableSizeWillChange:)])
    {
        [self.delegate appView: self drawableSizeWillChange: drawable_size];
    }
}

- (void) setUnsyncRefreshInterval: (NSTimeInterval) unsyncRefreshInterval
{
    if (_unsyncRefreshInterval == unsyncRefreshInterval)
        return;
    
    _unsyncRefreshInterval = unsyncRefreshInterval;
    
    if (self.redrawing && !self.vsyncEnabled)
    {
        [self.unsyncTimer invalidate];
        self.unsyncTimer = [NSTimer scheduledTimerWithTimeInterval:_unsyncRefreshInterval target:self selector:@selector(redraw) userInfo:nil repeats:YES];
    }
}

- (void) setVsyncEnabled: (BOOL) vsyncEnabled
{
    if (_vsyncEnabled == vsyncEnabled)
        return;
    
    BOOL was_redrawing = self.redrawing;
    self.redrawing = NO;
    _vsyncEnabled = vsyncEnabled;
    self.redrawing = was_redrawing;
}

- (void) setRedrawing: (BOOL) redrawing
{
    if (_redrawing == redrawing)
        return;
    
    _redrawing = redrawing;
    
    // Stop non-sync refresh timer if it's running
    if (self.unsyncTimer)
    {
        [self.unsyncTimer invalidate];
        self.unsyncTimer = nil;
    }

    // Enable/Disable redrawing on VSync
    if (_redrawing == YES)
    {
        if (_vsyncEnabled)
        {
            CVReturn cvReturn = CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
            assert(cvReturn == kCVReturnSuccess);
            
            cvReturn = CVDisplayLinkSetOutputCallback(_displayLink, &OnDisplayLinkFrame, (__bridge void *)self);
            assert(cvReturn == kCVReturnSuccess);
            
            cvReturn = CVDisplayLinkSetCurrentCGDisplay(_displayLink, CGMainDisplayID());
            assert(cvReturn == kCVReturnSuccess);
            
            CVDisplayLinkStart(_displayLink);
        }
        else
        {
            // Create non-sync refresh timer
            self.unsyncTimer = [NSTimer scheduledTimerWithTimeInterval:_unsyncRefreshInterval target:self selector:@selector(redraw) userInfo:nil repeats:YES];
        }
    }
    else if (_displayLink)
    {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
        CVDisplayLinkStop(_displayLink);
        CVDisplayLinkRelease(_displayLink);
        _displayLink = nil;
    }
}

- (void) redraw
{
    self.currentDrawable = [self.metalLayer nextDrawable];
    
    if ([self.delegate respondsToSelector:@selector(drawInView:)])
    {
        [self.delegate drawInView:self];
    }
}

- (void) windowWillClose:(NSNotification*)notification
{
    // Stop the display link when the window is closing because we will
    // not be able to get a drawable, but the display link may continue to fire
    if (notification.object == self.window && _displayLink)
    {
        CVDisplayLinkStop(_displayLink);
        CVDisplayLinkRelease(_displayLink);
        _displayLink = nil;
    }
}

- (void)setViewController:(NSViewController *)newController
{
    if (viewController)
    {
        NSResponder *controllerNextResponder = [viewController nextResponder];
        [super setNextResponder:controllerNextResponder];
        [viewController setNextResponder:nil];
    }
    
    viewController = newController;
    
    if (newController)
    {
        NSResponder *ownNextResponder = [self nextResponder];
        [super setNextResponder: viewController];
        [viewController setNextResponder:ownNextResponder];
    }
}

- (void)updateTrackingAreas
{
    if (trackingArea != nil)
    {
        [self removeTrackingArea:trackingArea];
        [trackingArea release];
    }
    
    const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
                                          NSTrackingActiveInKeyWindow |
                                          NSTrackingEnabledDuringMouseDrag |
                                          NSTrackingCursorUpdate |
                                          NSTrackingInVisibleRect |
                                          NSTrackingAssumeInside;
    
    trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                options:options
                                                  owner:self
                                               userInfo:nil];
    
    [self addTrackingArea:trackingArea];
    [super updateTrackingAreas];
}

- (void)setNextResponder:(NSResponder *)newNextResponder
{
    if (viewController)
    {
        [viewController setNextResponder:newNextResponder];
        return;
    }
    [super setNextResponder:newNextResponder];
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)canBecomeKeyView
{
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event
{
    return YES;
}

@end
