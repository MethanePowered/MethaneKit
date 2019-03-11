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

FILE: Methane/Platform/MacOS/AppViewController.mm
MacOS application view controller implementation.

******************************************************************************/

#import "AppViewController.h"

#include <Methane/Platform/MacOS/AppMac.h>
#include <Methane/Data/Types.h>

#include <string>

using namespace Methane;
using namespace Methane::Platform;

@implementation AppViewController
{
    Methane::Platform::AppMac* m_p_app;
    NSRect              m_frame_rect;
    bool                m_is_initialized;
    std::string         m_error;
}

- (id) initWithApp : (Methane::Platform::AppMac*) p_app andFrameRect : (NSRect) frame_rect
{
    self = [super init];
    if (!self)
        return nil;
    
    m_p_app = p_app;
    m_frame_rect = frame_rect;
    m_is_initialized = false;
    
    return self;
}

-(NSWindow*) window
{
    return m_p_app ? m_p_app->GetWindow() : nil;
}

- (Methane::Platform::AppMac*) getApp
{
    return m_p_app;
}

- (void) loadView
{
    assert(!!m_p_app);
    m_p_app->InitContext({ self }, { static_cast<uint32_t>(m_frame_rect.size.width), static_cast<uint32_t>(m_frame_rect.size.height) });
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self.view.window makeFirstResponder:self];
}

- (void)appView: (nonnull AppViewMT *) view drawableSizeWillChange: (CGSize)size
{
    assert(!!m_p_app);
    if (!m_is_initialized)
    {
        m_is_initialized = true;
        m_p_app->Init();
    }
    m_p_app->Resize( { static_cast<uint32_t>(size.width), static_cast<uint32_t>(size.height) }, false );
}

- (void) drawInView: (nonnull AppViewMT *) view
{
    assert(!!m_p_app);
    if (!m_is_initialized)
    {
        m_is_initialized = true;
        m_p_app->Init();
    }
    m_p_app->Update();
    m_p_app->Render();
}

- (void) keyDown:(NSEvent *)theEvent
{
    unichar characterHit = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    if (characterHit == 'q' || characterHit == 27)
    {
        [NSApp terminate:self];
    }
    else
    {
        [super keyDown:theEvent];
    }
}

@end
