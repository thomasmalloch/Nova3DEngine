# Nova3D Engine

A portal-based software renderer written in C++. It draws textured walls,
floors and ceilings with per-pixel lighting and distance fog. Every pixel is
written by the CPU. There is no GPU shader work and no 3D API. SFML is used
only to open a window and copy the finished pixel buffer to the screen.

The repository also contains a Windows Forms level editor that writes the
`.map` files the engine reads.

<!-- TODO: add screenshots here.
     Suggested: one wide shot of a lit room, one of a portal view,
     and one of the editor with a multi-node level open. -->

## Background

This project started as a Ludum Dare 44 entry,
[Escape from Staatliche Münze 3D](https://ldjam.com/events/ludum-dare/44/escape-from-staatliche-munze-3d).
The jam version drew flat-coloured walls only. The engine was rewritten and
extended after the jam with:

- Texture mapping on walls, floors and ceilings
- Fully 3D projected floors and ceilings, not the usual flat horizontal bands
- Per-pixel coloured point lights
- Per-pixel exponential distance fog that coloured lights can cut through
- A depth buffer, so geometry can be submitted in any order

The design is based on ideas from Bisqwit's portal renderer videos and
OneLoneCoder's 3D software renderer videos, reimplemented rather than copied.

All of the code here was written by hand, before any AI coding assistance
existed.

## Design goal

The C++ side is deliberately kept to a small number of files. The long-term
goal is a single header that a user can drop into a project, subclass, and
run, in the same spirit as OneLoneCoder's `olcPixelGameEngine.h`. Everything
in `NovaEngine.cpp`, `Camera.cpp` and `Geometry.cpp` is intended to fold back
into `Nova3DEngine.h` once the API stops moving.

## Features

| Feature | Notes |
| --- | --- |
| Portal rendering | Rooms are convex polygons ("nodes") linked by portal walls. The renderer recurses through portals and clips each room to the screen window of the portal it was seen through. |
| Textured walls | Perspective-correct `u`, affine `v` per vertical slice. Textures tile with `fmod`. |
| 3D floors and ceilings | Room polygons are clipped in view space, projected, fan-triangulated and scanline-filled with perspective-correct UVs. |
| Per-pixel lighting | Coloured point lights with inverse-square falloff, evaluated at the world position of each texel. |
| Distance fog | Exponential falloff. Light luminance is subtracted from the fog amount, so lit surfaces stay visible further away. |
| Depth buffer | One float per pixel. Lets floors, ceilings and walls be drawn in any order. |
| Alpha masking | Texels with `alpha == 0` are skipped and do not write depth. |
| Minimap | Debug overlay showing walls, portals, clipped geometry, and the camera frustum. |
| Level editor | Grid-based C# editor with node, wall, portal, actor and player placement. |

## Repository layout

```
Nova3DEngine/       The engine. Builds as a static library (.lib).
  Nova3DEngine.h      All engine types. The intended single-header target.
  NovaEngine.cpp      Frame loop, portal recursion, both rasterizers, shading.
  Camera.cpp          View parameters, FOV, clipping planes, projection.
  Geometry.cpp        Wall, Node and Map types, plus the .map file parser.

Nova3DEngineTest/   The demo. Builds as an executable, references the engine.
  Nova3DEngineTest.cpp  The whole reference game in one file.
  framework.h           Win32 includes.
  targetver.h           SDK version selection.
  Resource.h            Generated resource IDs.

Nova3DMapEditor/    C# WinForms level editor
  Form1.cs            Editing logic, .map save and load.
  Form1_Designer.cs   Generated layout code.
  Node.cs             A room. Holds walls, floor height, ceiling height.
  Wall.cs             A line segment with a colour.
  Portal.cs           A wall that also stores a destination node index.
  Actor.cs            A placed entity with position, type index and angle.
  Program.cs          Entry point and StringBuilder helpers.
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md) — how a frame is rendered, step by step
- [Engine API](docs/ENGINE_API.md) — how to build a game on top of the engine
- [Map format](docs/MAP_FORMAT.md) — the `.map` text format
- [Level editor](docs/EDITOR.md) — how to use the editor

## Building

### Engine

The solution has two projects:

- **`Nova3DEngine`** — static library (`.lib`). No `main`.
- **`Nova3DEngineTest`** — Windows-subsystem executable. Holds a project
  reference to the engine, and is what links SFML.

Build `Nova3DEngineTest`; it pulls the engine in as a dependency.

Requirements:

- Visual Studio 2022, toolset **v143**. The project was created under VS2019
  (`VCProjectVersion 16.0`) and later retargeted; VS2019 works if you
  retarget to v142.
- Windows 10 SDK (`WindowsTargetPlatformVersion 10.0`)
- [SFML 2.5.1](https://www.sfml-dev.org/), built for the matching
  architecture. `SFML_STATIC` is defined, so the **static** SFML libraries
  are expected and no DLLs need to be copied.
- No C++ language standard is set, so the MSVC default applies.
  `/permissive-` is on (`ConformanceMode`).

Configurations: Debug and Release, for both Win32 and x64.

Steps:

1. Install SFML 2.5.1 (static libs) for your target architecture.
2. **Fix the include paths.** They are currently absolute paths to a `J:`
   drive from the original machine, set per-configuration in both
   `IncludePath` and `AdditionalIncludeDirectories`. Point them at your own
   SFML install.
3. Build `Nova3DEngineTest`.
4. Put `wall.png`, `floor.png`, `ceiling.png` and a `.map` file in the
   working directory before running.

The demo's link list, for reference if you build your own game project:

```
Nova3DEngine.lib
sfml-graphics-s.lib  sfml-window-s.lib  sfml-system-s.lib  sfml-audio-s.lib
opengl32.lib  freetype.lib  winmm.lib  gdi32.lib
openal32.lib  flac.lib  vorbisenc.lib  vorbisfile.lib  vorbis.lib  ogg.lib
```

Debug uses the `-s-d` variants of the four SFML libraries.

Compiler settings worth knowing about:

- **AVX2 is enabled** (`EnableEnhancedInstructionSet`) in every
  configuration. This is what the `immintrin.h` and `xmmintrin.h` includes
  are for. There are no hand-written intrinsics yet, so the benefit comes
  from auto-vectorization in the rasterizer loops. It also means the binary
  will not run on pre-2013 CPUs.
- **OpenMP is enabled on Release x64 only.** Not on the other three
  configurations, and no `#pragma omp` directives exist in the source yet.
- Release uses `MaxSpeed`, `FavorSizeOrSpeed=Speed`, whole program
  optimization and `InlineFunctionExpansion=AnySuitable`, which matters
  because `DrawPixel` is declared `inline` and is the per-pixel hot path.
- The demo renders at pixel scale **3 in Debug** and **2 in Release**, since
  the Debug build skips no rasterizer work and needs the smaller buffer to
  stay interactive.

Note: in Debug builds, `DrawPixel` skips lighting and fog and writes the raw
texel. This is intentional and makes texture and geometry problems easier to
see. Build in Release to see the shading.

### Editor

Open the editor project in Visual Studio and build. It targets .NET Framework
with Windows Forms and has no external dependencies.

## Running the demo

`Nova3DEngineTest` is the reference game and the harness the renderer was
developed against. Build it and run it to see the engine working.

It needs four files in the working directory:

| File | Purpose |
| --- | --- |
| `wall.png` | Wall texture, 16x16 world units |
| `floor.png` | Floor texture, 100x100 world units |
| `ceiling.png` | Ceiling texture, 100x100 world units |
| a `.map` file | The level. Defaults to `lighttest.map`. |

Pass a different map filename as the command line argument to load it instead.

Controls:

| Key | Action |
| --- | --- |
| `W` / `S` | Forward and back |
| `A` / `D` | Strafe |
| Left / Right | Turn |
| Numpad 4 / 6 | Widen and narrow the field of view |
| Numpad 5 | Reset field of view to 90 degrees |

There is no collision, so the camera walks through solid walls. Portal
traversal still works correctly, because it only tests portal walls.

## Quick start

Subclass `nova::NovaEngine` and implement the four virtual methods, then call
`Setup` and `Run`. `Nova3DEngineTest.cpp` is a complete working example;
[Engine API](docs/ENGINE_API.md) walks through it section by section.

```cpp
class Game : public nova::NovaEngine
{
public:
    const std::string& Title() const override { return title_; }
    void UserLoad() override;                 // load map, textures, set camera node
    void UserUnload() override;               // free your resources
    void UpdateWorker(sf::Time delta, nova::UserInputManager& input) override;
    nova::IActor* ConvertActorFromID(const nova::IActor& actor) override;

private:
    std::string title_ = "My Game";
};

int main()
{
    Game game;
    if (!game.Setup(1280, 720, 2, false))
        return 1;

    game.Run();
    return 0;
}
```

## Known limitations

These are known and listed on purpose. They are the current edges of the
project, not hidden bugs.

**Renderer**

- `RenderPlanes` runs for every node in the map each frame, not only for
  visible ones. Floors and ceilings for rooms behind the camera are still
  clipped and often rejected, but the work is done. Restricting this to the
  set of nodes reached during portal recursion is the single biggest win
  available.
- Wall `v` coordinates are interpolated affinely down each vertical slice.
  This is correct for vertical walls but would need perspective correction
  if sloped walls were added.
- Texture sampling is nearest-neighbour. A code comment mentions bilinear
  filtering, which is a plan rather than a description.
- The far clipping plane is built but not pushed into the plane list. Draw
  distance is currently controlled by `fog_occlusion_distance_` instead.
- Top and bottom clipping planes are not used. Vertical bounds are handled
  by per-slice clamping instead.
- `depth_buffer_` is allocated at full window resolution but only the
  scaled-down `width_ * height_` region is used. Harmless, but wasteful.
- `RenderMap` skips walls with `node_index_ == 100`. This is a leftover
  sentinel from the jam version and should become a named constant or be
  removed.

**Engine**

- Actors are parsed from `.map` files but not stored or drawn.
  `RenderNodeActors` is an empty stub and the `Sprite` type it needs does not
  exist yet.
- Lights are hard-coded in `Setup` as four coloured lights. They belong in
  the map format.
- `ambient_light_` is parsed from the map but not used during shading.
- `MapLoader`, `SoundManager` and the `IUserInterfaceControl` hierarchy are
  declared but empty.
- `Matrix4x4` and `ProjectionMatrix` are written and correct but unused. The
  portal renderer projects directly instead.
- `UpdateActorPositions` currently only handles camera portal traversal. The
  name should change to match.
- `kRenderModeTile` is declared but the tile renderer is not implemented.
- `Title()` is never called. The window is created with an empty title and the
  frame rate counter overwrites it every second.

**Demo**

- `EngineTest::Title()` returns a reference to a local `std::string`. That is
  a dangling reference and undefined behaviour if ever used. It has not
  caused a crash only because nothing calls it. One-line fix.
- `Setup`'s return value is ignored, so a failed `UserLoad` would fall
  through into `Run` and dereference a null node.
- The spawn angle is nudged by `0.001f` to dodge a divide-by-zero when a
  sine or cosine lands on exactly zero. Guarding the `Slope` constructor
  would be the real fix.
- Wall `u` coordinates restart at 0 on every wall: `last_u` is never advanced
  past its initial value, so the wrap-around line below it does nothing.
  Invisible with a tiling texture, visible with a patterned one.
- SIMD headers are included and AVX2 is enabled project-wide, but there are
  no hand-written intrinsics yet. The rasterizer inner loops are the obvious
  candidates.
- OpenMP is enabled on Release x64 only, with no `#pragma omp` directives in
  the source. Either use it or turn it off.
- Build settings are inconsistent across the four configurations, and the
  SFML include paths are absolute paths to the original development machine.
  Anyone cloning the repo has to edit the `.vcxproj` before it builds.

**Editor**

- Node polygons are not validated for convexity or winding. The renderer
  assumes both.
- Wall textures are assigned in game code, not in the editor, so the editor
  cannot preview them.
- Floor and ceiling UV polygons (`plane_uv_`) are not exported. Game code
  must supply them.

## Roadmap

Roughly in order of value:

1. Restrict floor and ceiling rendering to nodes reached through portals.
2. Finish sprites and actors so the jam content can be rebuilt on this engine.
3. Move lights and textures into the map format and the editor.
4. Add convexity and winding validation to the editor.
5. Multi-thread the rasterizers by screen band, using the OpenMP support
   that is already switched on for Release x64.
6. Hand-vectorize the span loops in `RasterizePolygon` now that AVX2 is
   enabled.
7. Replace the absolute SFML and engine include paths with a property sheet
   or an environment variable so the solution builds on any machine.
8. Collapse the `.cpp` files back into `Nova3DEngine.h`.

## Credits

- Engine and editor: <!-- TODO: your name -->
- Renderer techniques: Bisqwit's portal rendering series and
  OneLoneCoder's software 3D rendering series
- Windowing, input and audio: [SFML](https://www.sfml-dev.org/)

## License

<!-- TODO: pick a license. MIT is a good default for a portfolio project. -->
