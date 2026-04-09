#pragma once

#if FUSION_ENABLE_TRACY
#include <tracy/Tracy.hpp>
#elifndef ZoneScoped
#define ZoneScoped
#endif

#include "Fusion/Misc/CoreDefines.h"
#include "Fusion/Misc/CoreMacros.h"
#include "Fusion/Misc/CoreTypes.h"
#include "Fusion/Templates/TypeTraits.h"
#include "Fusion/Templates/Templates.h"
#include "Fusion/Misc/Assert.h"

#include "Fusion/Algorithm/Hash.h"

#include "Fusion/Log/Log.h"

#include "Fusion/Math/Math.h"
#include "Fusion/Math/Vec2.h"
#include "Fusion/Math/Vec4.h"
#include "Fusion/Math/Mat4.h"
#include "Fusion/Math/Rect.h"
#include "Fusion/Math/Color.h"
#include "Fusion/Math/AffineTransform.h"

#include "Fusion/Containers/String.h"
#include "Fusion/Containers/Uuid.h"
#include "Fusion/Containers/Name.h"
#include "Fusion/Containers/Array.h"
#include "Fusion/Containers/StableDynamicArray.h"
#include "Fusion/Containers/HashMap.h"

#include "Fusion/Misc/Handle.h"
#include "Fusion/Misc/Exception.h"
#include "Fusion/Misc/MemoryUtils.h"

#include "Fusion/Misc/IntrusivePtr.h"

#include "Fusion/Image/Image.h"

#include "Fusion/Misc/RTTI.h"

#include "Fusion/Object/Ref.h"
#include "Fusion/Object/WeakRef.h"
#include "Fusion/Object/Object.h"

#include "Fusion/Delegates/Delegate.h"
#include "Fusion/Delegates/Signal.h"

