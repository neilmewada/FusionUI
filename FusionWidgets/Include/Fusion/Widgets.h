#pragma once

#include <Fusion/Core.h>
#include <Fusion/Platform.h>

#if FUSION_USE_SDL3
#include <Fusion/SDL3Platform.h>
#endif

#include <Fusion/RHI.h>

#if FUSION_USE_VULKAN
#include <Fusion/VulkanRHI.h>
#endif

#include <freetype/config/ftconfig.h>
#include <freetype/freetype.h>

#include "Macros.h"

// Application
#include "Application/Cursor.h"
#include "Application/ApplicationInstance.h"
#include "Application/Service.h"
#include "Application/Application.h"

// Event
#include "Event/Event.h"

// Layer
#include "Layer/Layer.h"
#include "Layer/LayerTree.h"

// Layout
#include "Layout/LayoutTypes.h"

// Style
#include "Style/Shape.h"
#include "Style/Gradient.h"
#include "Style/Pen.h"
#include "Style/Brush.h"
#include "Style/Font.h"
#include "Style/StyleState.h"
#include "Style/EasingCurve.h"
#include "Style/Transition.h"
#include "Style/Style.h"
#include "Style/Theme.h"
#include "Style/FusionCSS.h"

// Text Rendering
#include "Font/FontAtlas.h"

// Image Rendering
#include "Image/ImageAtlas.h"

// Timer
#include "Timer/Timer.h"

// Animation
#include "Animation/Animatable.h"
#include "Animation/Animation.h"
#include "Animation/TweenAnimation.h"
#include "Animation/SpringAnimation.h"
#include "Animation/AnimateBuilder.h"

// Painting
#include "Painting/Painter.h"

// Surface
#include "Surface/Surface.h"
#include "Surface/NativeSurface.h"

// Widgets
#include "Widget/Widget.h"
#include "Widget/CompoundWidget.h"
#include "Widget/DecoratedBox.h"
#include "Widget/ContainerWidget.h"
#include "Widget/Layout/StackBox.h"
#include "Widget/Label.h"
#include "Widget/Input/Button.h"
#include "Widget/Input/TextButton.h"
#include "Widget/Input/TextInput.h"

// Layouts
#include "Widget/Layout/ScrollBox.h"
#include "Widget/Layout/SplitBox.h"

