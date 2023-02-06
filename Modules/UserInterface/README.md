# Methane User Interface

## Modules

Code of these modules is located in `Methane::UserInterface` namespace:

- [Types](Types) - primitive user interface types like `UnitPoint`, `UnitRect`, `Context`, `Item`
- [Typography](Typography) - font rendering and text layout classes
- [Widgets](Widgets) - user interface widgets (work in progress)
- [App](App) - base user interface application class

## Intra-Domain Module Dependencies

```mermaid
flowchart TD;
    Types-->Typography
    Types-->Widgets
    Typography-->Widgets
    Types-->App
    Widgets-->App
```

## Cross-Domain Module Dependencies

```mermaid
flowchart TD;
    subgraph Data
        data_prov([Provider])
        data_prim([Primitives])
        data_event([Events])
    end
    
    subgraph Platform
        pal_input([Input])
        pal_app([App])
    end

    subgraph Graphics
        gfx_type([Types])
        gfx_rhi([RHI])
        gfx_app([App])
        gfx_prim([Primitives])
    end

    subgraph UserInterface
        ui_type([Types])
        ui_typo([Typography])
        ui_widget([Widgets])
        ui_app([App])
    end

    data_event-.->ui_type
    data_event-.->ui_typo
    gfx_type-.->ui_type
    gfx_rhi-.->ui_type
    data_prov-.->ui_typo
    data_prim-.->ui_typo
    gfx_type-.->ui_typo
    gfx_prim-.->ui_widget
    pal_input-.->ui_widget
    pal_app-.->ui_type
    gfx_app-.->ui_app
    ui_type-->ui_typo
    ui_type-->ui_widget
    ui_type-->ui_app
    ui_typo-->ui_widget
    ui_widget-->ui_app
```
