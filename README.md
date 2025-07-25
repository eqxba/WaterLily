# WaterLily
Vulkan graphics engine.

- Draw indirect / multi draw indirect.
- Meshlet pipeline (with task shader) with fallback to regular vertex pipeline. Meshoptimizer is used to generate meshlets.
- GPU culling (for now only frustum + screen size), GPU LOD selection.

Plans / in progress:
- 2-pass occlusion culling with visibility buffer both for meshes and individual meshlets (Alan Wake inspired).
- Fully bindless with both deferred / forward implementations (PBR lighting).
- Runtime GI

## Meshlets
![Meshlets](https://i.imgur.com/gp4JaDp.jpeg)

## GPU LOD selection
![GPU LOD selection](https://i.imgur.com/nGODW7u.jpeg)
