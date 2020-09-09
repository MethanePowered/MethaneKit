# Methane Instrumentation

Methane Kit contains integrated instrumentation of all libraries for performance analysis with trace collection.

| Tracy Frame Profiler | Intel Graphics Trace Analyzer |
| -------------------- | ----------------------------- |
| ![Asteroids Trace in Tracy](../../../Apps/Samples/Asteroids/Screenshots/AsteroidsWinTracyProfiling.jpg) | ![Asteroids Trace in GPA Trace Analyzer](../../../Apps/Samples/Asteroids/Screenshots/AsteroidsWinGPATraceAnalyzer.jpg) |

## Tracy Instrumentation

[Tracy Profiler](https://github.com/wolfpld/tracy)

### Profiling build options
- `METHANE_TRACY_PROFILING_ENABLED:BOOL=ON` - enables Tracy instrumentation and client connection
- `METHANE_TRACY_PROFILING_ON_DEMAND:BOOL=ON` - enable trace collection after Tracy profiler connection (otherwise from app start)
- `METHANE_GPU_INSTRUMENTATION_ENABLED:BOOL=ON` - enable GPU timestamp queries (affects performance)

### Instructions for analysis
1. Run Methane application built with Tracy profiling enabled or get `Profiling` [release build](https://github.com/egorodet/MethaneKit/releases)
2. Run [Tracy profiler v0.7.1](https://github.com/egorodet/Tracy/releases/tag/v0.7.1), run it from Terminal on MacOS
3. Click Methane application record in the Tracy connection dialog. Realtime trace collection begins.
  
## Intel ITT Instrumentation for Intel Graphics Trace Analyzer

[Intel Graphics Trace Analyzer](https://software.intel.com/en-us/gpa/graphics-trace-analyzer)

### Profiling build options
- `METHANE_ITT_INSTRUMENTATION_ENABLED:BOOL=ON` - enable ITT instrumentation
- `METHANE_ITT_METADATA_ENABLED:BOOL=ON` - enable metadata collection (like source paths and lines or frame numbers)

### Instructions for analysis
1. Start Graphics Monitor and configure trace options:
   - Click `Options` button, select `Trace` tab to change settings
   - Set the trace duration in seconds
   - In `GPA Domains` tab either select `Methane Kit` domain (to see Methane functions instrumentation) or `WinPixEventsRuntime` domain (to see command list debug group instrumentation) but not both - otherwise trace will display incorrectly.
2. On the `Desktop Applications` launcher screen: select `Trace` mode from combo-box in the right-bottom corner
3. Enter path to the Methane application executable built with ITT instrumentation enabled (any [release build](https://github.com/egorodet/MethaneKit/releases) can be used)
4. Click `Start` button to start application. Press `CTRL+SHIFT+T` to capture a trace of requested duration with events prior the current moment
5. Collected trace appears in the Graphics Monitor right-side list, double-click it to open.

### Other trace profiling build options
- `METHANE_SCOPE_TIMERS_ENABLED:BOOL=ON` - enable scope timer measurements (displayed on charts in the tools above)
- `METHANE_LOGGING_ENABLED:BOOL=ON` - enable logging (log messages are displayed in Tracy log)

## Scope-Timer for low-overhead profiling

[ScopeTimer](ScopeTimer.h) is a code primitive for low-overhead time measurement of some particular code scope
by adding macro-definitions to functions or other code scopes. This instrumentation is enabled with
`METHANE_SCOPE_TIMERS_ENABLED` build option.

```cpp
#include <Methane/ScopeTimer.h>

void Foo()
{
    META_FUNCTION_TIMER();
    for(size_t i = 0; i < 100500; ++i)
    {
        META_SCOPE_TIMER("Computation");
        ComputeSomething(i);
    }
}
```

Scope timers measure duration of the code scope by creating named `ScopeTimer` object on stack and saving 
duration between object construction and destruction in `ScopeTimer::Aggregator` singleton.
Aggregator accumulates scope timings and logs the results for all scopes when macros `META_SCOPE_TIMERS_FLUSH();` is called
or on application exit.

Additionally when scope timers are used together with ITT or Tracy instrumentation enabled, all scope timings are
added to charts displayed in Graphics Trace Analyzer or in Tracy Profiler.
