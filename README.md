# FusionUI

A cross-platform C++23 retained-mode UI library with declarative widgets, a type-safe stylesheet system, animations, transitions, and pluggable platform and rendering backends.

| Platform | Status | Platform Backend | Render Backend |
|-|-|-|-|
| Windows | ✅ | SDL3 | Vulkan |
| Mac | ✅ | SDL3 | MoltenVk |
| Linux | ❌ | ❌ | ❌ |

**Note**: FusionUI is in **experimental** stage, with future plans for linux support.

### ⭐ Feel free to star the project to show your support!

Join discord server here: https://discord.gg/TXGWUrFarx

[Quick Start](#quick-start) | [Building](#building) | [Features](#features)

![](./Screenshots/Sample2.png)

---

## Quick start

This is how you create a basic Fusion UI application:

```cpp
#include <Fusion/Core.h>
#include <Fusion/Widgets.h>

using namespace Fusion;

class MyWindow : public FDecoratedWidget
{
    FUSION_WIDGET(MyWindow, FDecoratedWidget)
public:
    void Construct() override
    {
        Super::Construct();

        Child(
            FNew(FButton)
            .Style("Button/Primary")
            .Height(32)
            .Width(160)
            .HAlign(EHAlign::Center)
            .VAlign(EVAlign::Center)
            .OnClick([this] { FUSION_LOG_INFO("UI", "Clicked!"); })
            .Child(
                FNew(FLabel)
                .Text("Hello Fusion")
                .Color(FColors::White)
                .HAlign(EHAlign::Center)
                .VAlign(EVAlign::Center)
            )
        );
    }
};

int main(int argc, char* argv[])
{
    FApplication app(argc, argv);

    Ref<FTheme> theme = app.CreateDefaultTheme();
    theme->MergeStyleSheet(FUSION_STYLE_SHEET
    {
        FUSION_STYLE(FButton, "Button/Primary", Shape, Background, Border)
        {
            Shape      = FRoundedRectangle(5.0f);
            Background = FColor(0.23f, 0.51f, 0.96f);
            Border     = FColor(0.16f, 0.40f, 0.82f);

            Transition(Background, FTransition::MakeTween(0.1f));

            FUSION_ON(Hovered)
            {
                Background = FColor(0.38f, 0.65f, 0.98f);
                Border     = FColor(0.26f, 0.53f, 0.90f);
            }

            FUSION_ON(Pressed, Hovered)
            {
                Background = FColor(0.11f, 0.31f, 0.85f);
                Border     = FColor(0.08f, 0.23f, 0.70f);
            }
        }
    });

    app.CreateMainWindow<MyWindow>();
    app.SetInitialWindowSize(800, 600);
    return app.Run();
}
```

This is what it looks like:

![](./Screenshots/Quickstart.png)

---

## Stylesheets with transitions

```cpp
FUSION_STYLE(FButton, "Button/Primary", Background, Border, Outline, OutlineOffset)
{
    Extends("Button/Base");
    Background = FColor(0.23f, 0.51f, 0.96f);

    Transition(Background, FTransition::MakeTween(0.1f, EEasingType::EaseOutCubic));
    Transition(OutlineOffset, FTransition::MakeSpring(180.0f, 18.0f));

    FUSION_ON(Hovered)
    {
        Background = FColor(0.38f, 0.65f, 0.98f);
    }

    FUSION_ON(FocusVisible)
    {
        Outline       = FPen::Solid(FColor(0.47f, 0.73f, 1.0f)).Thickness(2.0f);
        OutlineOffset = 2.5f;
    }
}
```

Hover fades in 100ms. The focus ring expands outward with spring physics on keyboard focus. Transitions fire automatically on state changes — no animation code in event handlers.

Properties are strongly typed — a typo is a compile error.

Both **tween** (duration + easing) and **spring** (stiffness + damping) transitions are supported. Interruption is automatic — a new transition picks up from the current in-progress value.

---

## Widget construction

```cpp
Child(
    FNew(FVerticalStack)
    .Spacing(10)
    .VAlign(EVAlign::Fill)
    (
        FNew(FButton)
        .Style("Button/Primary")
        .Height(32)
        .OnClick([this] { /* ... */ })
        .Child(
            FNew(FLabel)
            .Text("Click me")
            .Color(FColors::White)
        ),

        FNew(FTextInput)
        .Style("TextInput/Base")
        .Placeholder("Search...")
        .FillRatio(1.0f)
    )
);
```

---

## No global state

`FApplicationInstance` is a plain object — no singletons, no shared statics. Every service (animation, font atlas, style, renderer resources) lives inside the instance. Two application instances share zero state, which matters when loading multiple plugin instances in the same process or running parallel test contexts.

The rendering interface (`IFRenderBackend`) is public API — bring your own backend or use the Vulkan reference implementation that ships with the library. For embedding Fusion as a pass in your own frame graph, `FApplicationInstance` exposes `Tick()` directly.

---

## Building

**Requirements:** CMake 3.21+, C++23, Vulkan SDK, SDL3, FreeType

```bash
git clone https://github.com/your-username/FusionUI.git
cd FusionUI
cmake -B build -DFUSION_BUILD_SAMPLES=ON
cmake --build build
./build/Samples/FusionSample
```

| CMake option | Default | |
|---|---|---|
| `FUSION_BUILD_SAMPLES` | OFF | Sample application |
| `FUSION_BUILD_TESTS` | OFF | Unit tests |
| `FUSION_USE_SDL3` | ON | SDL3 windowing backend |
| `FUSION_USE_VULKAN` | ON | Vulkan render backend |
| `FUSION_ENABLE_TRACY` | OFF | Tracy profiler integration |

**Platforms:** Windows, macOS

**Dependencies:** [Vulkan SDK](https://vulkan.lunarg.com), [SDL3](https://github.com/libsdl-org/SDL), [FreeType](https://freetype.org), [xxHash](https://github.com/Cyan4973/xxHash), [cpptrace](https://github.com/jeremy-rifkin/cpptrace)

---

## Features

- **Styling** — named styles, state overrides, style inheritance, per-property transitions, all type-checked at compile time
- **Animation** — spring and tween, interruptible, fluent API (`FAnimate_Spring`, `FAnimate_Tween`)
- **Text** — SDF rendering via FreeType, crisp at any size; text input with selection, clipboard, word jumping, password masking
- **Painting** — paths, bezier curves, rounded rects, gradients, dashed/dotted strokes, gradient color along arc length, clipping
- **Focus** — full tab order, `FocusVisible` state (ring on keyboard navigation only, not mouse clicks)

---


