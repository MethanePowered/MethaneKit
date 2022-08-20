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

FILE: Methane/Platform/iOS/AppViewController.mm
iOS application view controller implementation.

******************************************************************************/

#import "AppViewController.hh"

#include <Methane/Platform/iOS/AppIOS.hh>
#include <Methane/Data/Types.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <string>

using namespace Methane;
using namespace Methane::Platform;

@implementation AppViewController
{
    AppMac*     m_p_app;
    CGRect      m_frame_rect;
    bool        m_is_initialized;
    std::string m_error;
}

- (id) initWithApp : (Methane::Platform::AppMac*) p_app andFrameRect : (CGRect) frame_rect
{
    META_FUNCTION_TASK();

    self = [super init];
    if (!self)
        return nil;
    
    m_p_app = p_app;
    m_frame_rect = frame_rect;
    m_is_initialized = false;
    
    return self;
}

-(nullable UIWindow*) window
{
    META_FUNCTION_TASK();
    return m_p_app ? m_p_app->GetWindow() : nil;
}

- (Methane::Platform::AppMac*) getApp
{
    return m_p_app;
}

- (void) loadView
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    m_p_app->InitContextWithErrorHandling({ self }, Data::FrameSize(m_frame_rect.size.width, m_frame_rect.size.height));
}

- (void)viewDidLoad
{
    META_FUNCTION_TASK();
    [super viewDidLoad];
}

- (void)appView: (nonnull AppViewMT *) view drawableSizeWillChange: (CGSize)size
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);

    if (!m_is_initialized)
    {
        m_is_initialized = m_p_app->InitWithErrorHandling();
    }
    m_frame_rect = [view frame];
    m_p_app->Resize( Data::FrameSize(size.width, size.height), false);
}

- (void) drawInView: (nonnull AppViewMT*) view
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(view)

    if (!m_is_initialized)
    {
        m_is_initialized = m_p_app->InitWithErrorHandling();
    }
    m_p_app->UpdateAndRenderWithErrorHandling();
}

@end
