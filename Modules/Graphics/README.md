# Methane Graphics

## Modules

Code of these modules is located in `Methane::Graphics` namespace:

- [Types](Types) - primitive graphics gfx_type like `Color`, `Point`, `Rect`, `Volume`.
- [Camera](Camera) - base perspective/orthogonal camera model, arc-ball camera and interactive action camera.
- [Mesh](Mesh) - procedural generated mesh data for quad, cube, sphere, icosahedron and uber-mesh.
- [RHI](RHI) - Rendering Hardware Interface, abstraction API for native graphic APIs (DirectX, Vulkan and Metal).
- [Primitives](Primitives) - graphics extensions like `ImageLoader`, `ScreenQuad`, `SkyBox`, `MeshBuffers`, etc.
- [App](App) - base graphics application class implementation.

## Intra-Domain Module Dependencies

```mermaid
graph TD;
    Types-->Camera;
    Types-->Mesh;
    Types-->RHI;
    RHI-->Primitives;
    Mesh-->Primitives;
    Camera-->App;
    Camera-->Primitives;
    Primitives-->App;
    RHI-->App;
```

## Cross-Domain Module Dependencies

```mermaid
graph TD;
    subgraph Data
        direction LR
        data_type([Types])
        data_prim([Primitives])
        data_anim([Animation])
        data_prov([Provider])
        data_event([Events])
        data_range([RangeSet])
    end
    
    subgraph Platform
        direction TB
        pal_util([Utils])
        pal_app([App])
        pal_input([Input])
    end

    subgraph Graphics
        gfx_type([Types])
        gfx_cam([Camera])
        gfx_mesh([Mesh])
        gfx_rhi([RHI])
        gfx_prim([Primitives])
        gfx_app([App])
    end
    
    data_anim-.->gfx_cam
    data_type-.->gfx_type
    gfx_type-->gfx_cam;
    gfx_type-->gfx_mesh;
    gfx_type-->gfx_rhi;
    gfx_cam-->gfx_prim;
    data_prov-.->gfx_prim
    data_prov-.->gfx_rhi
    gfx_mesh-->gfx_prim;
    data_prim-.->gfx_prim
    gfx_rhi-->gfx_prim;
    gfx_cam-->gfx_app;
    data_prov-.->gfx_app
    gfx_prim-->gfx_app;
    gfx_rhi-->gfx_app;
    pal_util-.->gfx_prim
    pal_app-.->gfx_app
    pal_input-.->gfx_app
    data_prim-.->gfx_rhi
    data_event-.->gfx_rhi
    data_range-.->gfx_rhi
    pal_util-.->gfx_rhi
```

## Unit Tests Coverage

See [Graphics Tests description](/Tests/Graphics/README.md) for details on unit tests coverage.

