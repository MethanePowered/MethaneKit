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
        Data::Provider
        Data::Primitives
        Data::Events
    end
    subgraph Platform
        Platform::Input
    end
    subgraph Graphics
        Graphics::Types
        Graphics::App
        Graphics::RHI
        Graphics::Primitives
    end
    subgraph UserInterface
        Types-->Typography
        Types-->Widgets
        Typography-->Widgets
        Types-->App
        Widgets-->App
    end
    Data::Events-->Types
    Graphics::Types-->Types
    Graphics::RHI-->Types
    Data::Provider-->Typography
    Data::Primitives-->Typography
    Graphics::Types-->Typography
    Graphics::Primitives-->Widgets
    Platform::Input-->Widgets
    Graphics::App-->App
```
