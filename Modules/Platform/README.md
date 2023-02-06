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
        data_type([Types])
        data_prov([Provider])
    end

    subgraph Platform
        pal_util([Utils])
        pal_key([Input/Keyboard])
        pal_mouse([Input/Mouse])
        pal_cont([Input/Controllers])
        pal_action([Input/ActionControllers])
        pal_view([AppView])
        pal_app([App])
    end

    pal_util-->pal_key
    pal_key-->pal_cont
    pal_mouse-->pal_cont
    pal_cont-->pal_action
    pal_action-->pal_app
    pal_view-->pal_app
    pal_util-->pal_app

    data_type-.->pal_key
    data_type-.->pal_mouse
    data_type-.->pal_view
    data_prov-.->pal_app
    data_type-.->pal_app
```
