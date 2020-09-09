# Methane Instrumentation

Methane Kit comes with integrated instrumentation of all libraries for convenient performance analysis using trace collection with the following tools.

| Tracy Frame Profiler | Intel Graphics Trace Analyzer |
| -------------------- | ----------------------------- |
| ![Asteroids Trace in Tracy](../../../Apps/Samples/Asteroids/Screenshots/AsteroidsWinTracyProfiling.jpg) | ![Asteroids Trace in GPA Trace Analyzer](../../../Apps/Samples/Asteroids/Screenshots/AsteroidsWinGPATraceAnalyzer.jpg) |

## Tracy Instrumentation

[Tracy Profiler](https://github.com/wolfpld/tracy) is a real time, nanosecond resolution, remote telemetry frame profiler
(free open-source alternative to RAD Telemetry). Tracy connects locally or remotely to the profiled application and 
instantly collects and displays trace data with low overhead.
See [Tracy user manual](https://github.com/wolfpld/tracy/releases/download/v0.7.1/tracy.pdf) for more details.

Methane Kit includes the following Tracy instrumentation:
- Frame delimiters after present call
- Zones of Methane function scopes
- Mutex locks instrumentation
- Tracy messages on methane logging
- Dynamic memory allocations with new/delete operators instrumentation
- Call-stacks collection for memory allocation and optionally for function zones
- Custom plots displaying time measurements collected with [scope timers](#scope-timer-primitive)
- GPU Context zones for command lists execution on GPU in scope of command queues
- Thread names

### Profiling build options
- `METHANE_TRACY_PROFILING_ENABLED:BOOL=ON` - enables Tracy instrumentation and client connection
- `METHANE_TRACY_PROFILING_ON_DEMAND:BOOL=ON` - enable trace collection after Tracy profiler connection (otherwise from app start)
- `METHANE_GPU_INSTRUMENTATION_ENABLED:BOOL=ON` - enable GPU timestamp queries displayed on GPU context tracks
- `METHANE_SCOPE_TIMERS_ENABLED` - enable measuring custom scope timings and displaying as plots in Tracy
- `METHANE_LOGGING_ENABLED:BOOL=ON` - enable logging displayed in Tracy log and in timeline markers

### Instructions for analysis
1. Run Methane application built with Tracy profiling enabled or get `Profiling` [release build](https://github.com/egorodet/MethaneKit/releases)
2. Run [Tracy profiler v0.7.1](https://github.com/egorodet/Tracy/releases/tag/v0.7.1) (run executable from Terminal on MacOS)
3. Click Methane application record in the Tracy connection dialog. Realtime trace collection starts instantly.
  
## Intel ITT Instrumentation

[Intel Graphics Trace Analyzer](https://software.intel.com/en-us/gpa/graphics-trace-analyzer) is an free offline trace capture and analysis tool,
with advanced visualization of low-level Windows graphics events (including command queues execution in user and kernel mode drivers).
Application events are collected with [Intel ITT](https://software.intel.com/content/www/us/en/develop/articles/intel-itt-api-open-source.html) instrumentation API 
also supported by [Intel VTune Profiler](https://software.intel.com/content/www/us/en/develop/tools/vtune-profiler.html).

Methane Kit includes the following ITT instrumentation:
- Scope tasks for all Methane functions with optional metadata of source file path and line number
- Process markers for frame delimiters after present calls with optional metadata including frame index and frame buffer index
- Thread markers for mouse and keyboard input events with optional metadata of pressed keys/buttons and mouse position 
- Counters for custom scope timing measurements with [scope timers](#scope-timer-primitive)
- Thread names

Various GPU events and metrics are collected by [Intel GPA](https://software.intel.com/content/www/us/en/develop/tools/graphics-performance-analyzers.html) automatically.

### Profiling build options
- `METHANE_ITT_INSTRUMENTATION_ENABLED:BOOL=ON` - enable ITT instrumentation
- `METHANE_ITT_METADATA_ENABLED:BOOL=ON` - enable metadata collection (source paths and lines or frame numbers)
- `METHANE_SCOPE_TIMERS_ENABLED` - enable measuring custom scope timings collection and displaying as charts in Trace Analyzer

### Instructions for analysis
1. Start Intel Graphics Monitor from [Intel GPA installation](https://software.intel.com/content/www/us/en/develop/tools/graphics-performance-analyzers.html) and configure Trace options:
    - Click `Options` button, select `Trace` tab to change settings
    - Set the trace duration in seconds
    - In `GPA Domains` tab either select `Methane Kit` domain (to see Methane functions instrumentation) or `WinPixEventsRuntime` domain (to see command list debug group instrumentation) but not both - otherwise trace will display incorrectly.
2. On the `Desktop Applications` launcher screen: select `Trace` mode from combo-box in the right-bottom corner
3. Enter path to the Methane application executable built with ITT instrumentation enabled (any [release build](https://github.com/egorodet/MethaneKit/releases) can be used)
4. Click `Start` button to start application. Press `CTRL+SHIFT+T` to capture a trace of requested duration with events prior the current moment
5. Collected trace appears in the Graphics Monitor right-side list, double-click it to open.

## Scope Timer primitive

[ScopeTimer](ScopeTimer.h) is a code primitive for low-overhead time measurement of some particular code scope
by adding macro-definitions to functions or other code scopes. This instrumentation is enabled with
`METHANE_SCOPE_TIMERS_ENABLED:BOOL=ON` build option.

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
Aggregator accumulates scope timings and logs the results for all scopes to the debug output 
when macros `META_SCOPE_TIMERS_FLUSH();` is called or application exits.

Additionally when scope timers are used together with ITT or Tracy instrumentation enabled, all scope timings are
added to charts displayed in Graphics Trace Analyzer or in Tracy Profiler.
