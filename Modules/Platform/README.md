# Methane Platform

## Modules

Code of these modules is located in `Methane::Platform` namespace:

- [AppView](AppView) - application view and environment platform-abstraction classes
- [App](App) - application platform-abstraction class and platform-specific implementations
- [Input](Input) - application input with mouse and keyboard and platform-specific handling implementations.
- [Utils](Utils) - platform utilities

## Intra-Domain Modules Dependencies

```mermaid
graph TD;
    Utils-->Input/Keyboard;
    Input/Keyboard-->Input/Controllers;
    Input/Mouse-->Input/Controllers;
    Input/Controllers-->Input/ActionControlls;
    Input/ActionControlls-->App
    AppView-->App
    Utils-->App
```

## Cross-Domain Modules Dependencies

```mermaid
graph TD;
    subgraph Data
        Data::Types
        Data::Provider
    end
    subgraph Platform
        Utils-->Input/Keyboard;
        Input/Keyboard-->Input/Controllers;
        Input/Mouse-->Input/Controllers;
        Input/Controllers-->Input/ActionControlls;
        Input/ActionControlls-->App
        AppView-->App
        Utils-->App
    end
    Data::Types-->Utils;
    Data::Types-->Input/Keyboard;
    Data::Types-->Input/Mouse;
    Data::Types-->AppView
    Data::Types-->App
    Data::Provider-->App
```
