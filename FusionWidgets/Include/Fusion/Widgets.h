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

#include "Macros.h"

#include "Application/ApplicationInstance.h"
#include "Application/Service.h"
#include "Application/Application.h"

// Layer
#include "Layer/Layer.h"
#include "Layer/LayerTree.h"

// Style
#include "Style/Gradient.h"
#include "Style/Pen.h"
#include "Style/Brush.h"

// Painting
#include "Painting/Painter.h"

// Surface
#include "Surface/Surface.h"
#include "Surface/NativeSurface.h"

// Widgets
#include "Widget/Widget.h"
