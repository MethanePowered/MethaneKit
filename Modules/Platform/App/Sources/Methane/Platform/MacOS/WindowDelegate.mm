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

FILE: Methane/Platform/MacOS/AppDelegate.mm
MacOS application delegate implementation.

******************************************************************************/

#import "WindowDelegate.hh"

#include <Methane/Platform/MacOS/AppMac.hh>
#include <Methane/Data/Instrumentation.h>

#include <cassert>

using namespace Methane;
using namespace Methane::Platform;

@implementation WindowDelegate
{
    AppMac* m_p_app;
}

- (id) initWithApp : (AppMac*) p_app
{
    ITT_FUNCTION_TASK();

    self = [super init];
    if (!self)
        return nil;

    m_p_app = p_app;
    
    return self;
}

- (void) windowDidEnterFullScreen:(NSNotification *)notification
{
    ITT_FUNCTION_TASK();
    assert(!!m_p_app);

    m_p_app->SetFullScreen(true);
}

- (void) windowDidExitFullScreen:(NSNotification *)notification
{
    ITT_FUNCTION_TASK();
    assert(!!m_p_app);

    m_p_app->SetFullScreen(false);
}

@end
