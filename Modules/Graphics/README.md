# Methane Graphics Modules

Code of these modules is located in `Methane::Graphics` namespace:

- [Types](Types) - primitive graphics types like `Color`, `Point`, `Rect`, `Volume`.
- [Camera](Camera) - base perspective/orthogonal camera model, arc-ball camera and interactive action camera.
- [Mesh](Mesh) - procedural generated mesh data for quad, cube, sphere, icosahedron and uber-mesh.
- [RHI](RHI) - Rendering Hardware Interface, abstraction API for native graphic APIs (DirectX, Vulkan and Metal).
- [Primitives](Primitives) - graphics extensions like `ImageLoader`, `ScreenQuad`, `SkyBox`, `MeshBuffers`, etc.
- [App](App) - base graphics application class implementation.

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
