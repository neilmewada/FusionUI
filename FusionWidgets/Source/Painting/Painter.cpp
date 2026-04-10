#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	// Credit: Dear ImGui
	static constexpr f32 MinCircleRadius = 0.1f;

	static FVec2 ImBezierCubicCalc(const FVec2& p1, const FVec2& p2, const FVec2& p3, const FVec2& p4, float t)
	{
		float u = 1.0f - t;
		float w1 = u * u * u;
		float w2 = 3 * u * u * t;
		float w3 = 3 * u * t * t;
		float w4 = t * t * t;
		return FVec2(w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x, w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y);
	}

	static FVec2 ImBezierQuadraticCalc(const FVec2& p1, const FVec2& p2, const FVec2& p3, float t)
	{
		float u = 1.0f - t;
		float w1 = u * u;
		float w2 = 2 * u * t;
		float w3 = t * t;
		return FVec2(w1 * p1.x + w2 * p2.x + w3 * p3.x, w1 * p1.y + w2 * p2.y + w3 * p3.y);
	}

	static void PathBezierCubicCurveToCasteljau(FPainter* painter,
		float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tess_tol, int level)
	{
		float dx = x4 - x1;
		float dy = y4 - y1;
		float d2 = (x2 - x4) * dy - (y2 - y4) * dx;
		float d3 = (x3 - x4) * dy - (y3 - y4) * dx;
		d2 = (d2 >= 0) ? d2 : -d2;
		d3 = (d3 >= 0) ? d3 : -d3;
		if ((d2 + d3) * (d2 + d3) < tess_tol * (dx * dx + dy * dy))
		{
			painter->PathInsert(FVec2(x4, y4));
		}
		else if (level < 10)
		{
			float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
			float x23 = (x2 + x3) * 0.5f, y23 = (y2 + y3) * 0.5f;
			float x34 = (x3 + x4) * 0.5f, y34 = (y3 + y4) * 0.5f;
			float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
			float x234 = (x23 + x34) * 0.5f, y234 = (y23 + y34) * 0.5f;
			float x1234 = (x123 + x234) * 0.5f, y1234 = (y123 + y234) * 0.5f;
			PathBezierCubicCurveToCasteljau(painter, x1, y1, x12, y12, x123, y123, x1234, y1234, tess_tol, level + 1);
			PathBezierCubicCurveToCasteljau(painter, x1234, y1234, x234, y234, x34, y34, x4, y4, tess_tol, level + 1);
		}
	}

	static void PathBezierQuadraticCurveToCasteljau(FPainter* painter,
		float x1, float y1, float x2, float y2, float x3, float y3, float tess_tol, int level)
	{
		float dx = x3 - x1, dy = y3 - y1;
		float det = (x2 - x3) * dy - (y2 - y3) * dx;
		if (det * det * 4.0f < tess_tol * (dx * dx + dy * dy))
		{
			painter->PathInsert(FVec2(x3, y3));
		}
		else if (level < 10)
		{
			float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
			float x23 = (x2 + x3) * 0.5f, y23 = (y2 + y3) * 0.5f;
			float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
			PathBezierQuadraticCurveToCasteljau(painter, x1, y1, x12, y12, x123, y123, tess_tol, level + 1);
			PathBezierQuadraticCurveToCasteljau(painter, x123, y123, x23, y23, x3, y3, tess_tol, level + 1);
		}
	}

	FPainter::FPainter(FLayer* layer) : m_Layer(layer)
	{
		m_DrawList = &layer->m_DrawList;
		m_DpiScale = layer->GetDpiScale();
		
		m_Surface = layer->GetWidgetSurface().Get();
		FUSION_ASSERT(m_Surface != nullptr, "Could not get widget's surface in FPainter.");

		m_Application = m_Surface->GetApplication().Get();
		FUSION_ASSERT(m_Application != nullptr, "Could not get application reference in FPainter.");

		m_DrawList->fringeScale = 1.0f;

		for (int i = 0; i < kArcFastTableSize; i++)
		{
			const float a = ((float)i * 2 * (float)M_PI) / (float)kArcFastTableSize;
			m_ArcFastVertex[i] = FVec2(cosf(a), sinf(a));
		}

		m_ArcFastRadiusCutoff = ((m_CircleSegmentMaxError) / (1 - cosf(FMath::PI / FMath::Max((float)(kArcFastTableSize), FMath::PI))));
	}

	void FPainter::ResetState()
	{
		m_CurrentBrush = {};
		m_CurrentPen = {};
		PathClear();
		m_AntiAliased = true;
	}

	void FPainter::PushTransform(const FAffineTransform& transform)
	{
		m_TransformStack.Insert(GetCurrentTransform() * transform);
	}

	void FPainter::PopTransform()
	{
		FUSION_ASSERT(!m_TransformStack.IsEmpty(), "PopTransform() called on empty stack.");
		m_TransformStack.RemoveLast();
	}

	FAffineTransform FPainter::GetCurrentTransform()
	{
		return m_TransformStack.IsEmpty() ? FAffineTransform::Identity() : m_TransformStack.Last();
	}

	void FPainter::PathClear()
	{
		ZoneScoped;

		m_Path.RemoveAll();

		m_PathMin = FVec2(FNumericLimits<f32>::Max(), FNumericLimits<f32>::Max());
		m_PathMax = FVec2(FNumericLimits<f32>::Min(), FNumericLimits<f32>::Min());
	}

	void FPainter::PathInsert(FVec2 point)
	{
		ZoneScoped;

		point = GetCurrentTransform().TransformPoint(point);

		m_Path.Insert(point);

		PathMinMax(point);
	}

	void FPainter::PathArcTo(const FVec2& center, float radius, float startAngle, float endAngle)
	{
		ZoneScoped;

		if (radius < MinCircleRadius)
		{
			PathInsert(center);
			return;
		}

		const float arcLength = FMath::Abs(endAngle - startAngle);
		const int circleSegmentCount = CalculateNumCircleSegments(radius);
		const int arcSegmentCount = FMath::Max(FMath::RoundToInt(ceilf(circleSegmentCount * arcLength / (FMath::PI * 2.0f))),
			FMath::RoundToInt(2.0f * FMath::PI / arcLength));

		PathArcTo(center, radius, startAngle, endAngle, arcSegmentCount);
	}

	void FPainter::PathArcToFast(const FVec2& center, float radius, float startAngle, float endAngle)
	{
		ZoneScoped;

		if (radius < MinCircleRadius)
		{
			PathInsert(center);
			return;
		}

		int sampleMin = (int)FMath::Round(startAngle / (FMath::PI * 2.0f) * kArcFastTableSize);
		int sampleMax = (int)FMath::Round(endAngle / (FMath::PI * 2.0f) * kArcFastTableSize);

		PathArcToFastInternal(center, radius, sampleMin, sampleMax, 0);
	}

	void FPainter::PathBezierCubicCurveTo(const FVec2& p1, const FVec2& p2, const FVec2& p3, const FVec2& p4,
		int numSegments)
	{
		if (numSegments == 0)
		{
			// Auto-tessellated
			PathBezierCubicCurveToCasteljau(this, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, m_CurveTessellationTolerance, 0);
		}
		else
		{
			float totalSteps = 1.0f / (float)numSegments;
			for (int i = 1; i <= numSegments; i++)
			{
				PathInsert(ImBezierCubicCalc(p1, p2, p3, p4, totalSteps * i));
			}
		}
	}

	void FPainter::PathQuadraticCubicCurveTo(const FVec2& p1, const FVec2& p2, const FVec2& p3, int numSegments)
	{
		if (numSegments == 0)
		{
			// Auto-tessellated
			PathBezierQuadraticCurveToCasteljau(this, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, m_CurveTessellationTolerance, 0);
		}
		else
		{
			float totalSteps = 1.0f / (float)numSegments;
			for (int i = 1; i <= numSegments; i++)
			{
				PathInsert(ImBezierQuadraticCalc(p1, p2, p3, totalSteps * i));
			}
		}
	}

	void FPainter::PathRect(const FRect& rect, const FVec4& cornerRadius)
	{
		const FVec2& min = rect.min;
		const FVec2& max = rect.max;

		if (cornerRadius.GetMax() < 0.5f)
		{
			PathInsert(min);
			PathInsert(FVec2(max.x, min.y));
			PathInsert(max);
			PathInsert(FVec2(min.x, max.y));
		}
		else
		{
			PathArcToFast(FVec2(min.x + cornerRadius.topLeft, min.y + cornerRadius.topLeft),
				cornerRadius.topLeft, FMath::PI, FMath::PI * 1.5f);
			PathArcToFast(FVec2(max.x - cornerRadius.topRight, min.y + cornerRadius.topRight),
				cornerRadius.topRight, FMath::PI * 1.5f, FMath::PI * 2.0f);
			PathArcToFast(FVec2(max.x - cornerRadius.bottomRight, max.y - cornerRadius.bottomRight),
				cornerRadius.bottomRight, 0.0f, FMath::PI * 0.5f);
			PathArcToFast(FVec2(min.x + cornerRadius.bottomLeft, max.y - cornerRadius.bottomLeft),
				cornerRadius.bottomLeft, FMath::PI * 0.5f, FMath::PI);
		}
	}

	bool FPainter::PathFill()
	{
		bool result = PathFillInternal();
		PathClear();
		return result;
	}

	bool FPainter::PathStroke(bool closed)
	{
		bool result = PathStrokeInternal(closed);
		PathClear();
		return result;
	}

	bool FPainter::PathFillAndStroke()
	{
		bool result = PathFillInternal();
		result |= PathStrokeInternal(true);
		PathClear();
		return result;
	}

	void FPainter::StrokeRect(const FRect& rect, const FVec4& cornerRadius)
	{
		PathClear();
		PathRect(rect, cornerRadius);
		PathStroke(true);
	}

	void FPainter::FillRect(const FRect& rect, const FVec4& cornerRadius)
	{
		PathClear();
		PathRect(rect, cornerRadius);
		PathFill();
	}

	void FPainter::FillAndStrokeRect(const FRect& rect, const FVec4& cornerRadius)
	{
		PathClear();
		PathRect(rect, cornerRadius);
		PathFillAndStroke();
	}

	void FPainter::StrokeCircle(const FVec2& center, f32 radius)
	{
		PathClear();
		PathArcToFast(center, radius, 0, FMath::PI * 2.0f);
		PathStroke(true);
	}

	void FPainter::FillCircle(const FVec2& center, f32 radius)
	{
		PathClear();
		PathArcToFast(center, radius, 0, FMath::PI * 2.0f);
		PathFill();
	}

	void FPainter::FillAndStrokeCircle(const FVec2& center, f32 radius)
	{
		PathClear();
		PathArcToFast(center, radius, 0, FMath::PI * 2.0f);
		PathFillAndStroke();
	}

	void FPainter::DrawLine(const FVec2& p1, const FVec2& p2)
	{
		PathClear();
		PathInsert(p1);
		PathInsert(p2);
		PathStroke(false);
	}

	void FPainter::StrokeShape(const FRect& rect, const FShape& shape)
	{
		switch (shape.GetShapeType())
		{
		case EShapeType::None:
			return;
		case EShapeType::Rect:
			StrokeRect(rect, FVec4());
			break;
		case EShapeType::RoundedRect:
			StrokeRect(rect, shape.GetCornerRadius());
			break;
		case EShapeType::Circle:
		{
			FVec2 center = rect.min + rect.GetSize() / 2.0f;
			f32 radius = rect.GetSize().GetMin() / 2.0f;
			StrokeCircle(center, radius);
		}
		break;
		}
	}

	void FPainter::FillShape(const FRect& rect, const FShape& shape)
	{
		switch (shape.GetShapeType())
		{
		case EShapeType::None:
			return;
		case EShapeType::Rect:
			FillRect(rect, FVec4());
			break;
		case EShapeType::RoundedRect:
			FillRect(rect, shape.GetCornerRadius());
			break;
		case EShapeType::Circle:
		{
			FVec2 center = rect.min + rect.GetSize() / 2.0f;
			f32 radius = rect.GetSize().GetMin() / 2.0f;
			FillCircle(center, radius);
		}
		break;
		}
	}

	void FPainter::FillAndStrokeShape(const FRect& rect, const FShape& shape)
	{
		switch (shape.GetShapeType())
		{
		case EShapeType::None:
			return;
		case EShapeType::Rect:
			FillAndStrokeRect(rect, FVec4());
			break;
		case EShapeType::RoundedRect:
			FillAndStrokeRect(rect, shape.GetCornerRadius());
			break;
		case EShapeType::Circle:
		{
			FVec2 center = rect.min + rect.GetSize() / 2.0f;
			f32 radius = rect.GetSize().GetMin() / 2.0f;
			FillAndStrokeCircle(center, radius);
		}
		break;
		}
	}

	void FPainter::PushClip(const FRect& rect, const FShape& shape)
	{
		int index = (int)m_DrawList->clipRectArray.GetCount();

		m_ClipStack.Insert(index);

		FVec4 radii = shape.GetCornerRadius();
		FVec2 halfSize = rect.GetSize() / 2.0f;

		if (shape.GetShapeType() == EShapeType::Rect)
		{
			radii = FVec4();
		}
		else if (shape.GetShapeType() == EShapeType::Circle)
		{
			radii = FVec4();
			f32 min = halfSize.GetMin();
			halfSize = FVec2(min, min);
		}

		FVec2 clipCenter = rect.min + halfSize;

		m_DrawList->clipRectArray.Insert(FUIClipRect{
			.clipInverseTransform = (GetCurrentTransform() * FAffineTransform::Translation(clipCenter)).ToMatrix4x4().GetInverse(),
			.cornerRadii = radii,
			.clipHalfSize = halfSize
			});
	}

	void FPainter::PopClip()
	{
		m_ClipStack.RemoveLast();
	}

	int FPainter::CalculateNumCircleSegments(float radius) const
	{
		//const int radiusIndex = (int)(radius + 0.999999f); // ceil to never reduce accuracy
		return FMath::Clamp((((((int)ceilf(FMath::PI / acosf(1 - FMath::Min((m_CircleSegmentMaxError), (radius)) / (radius)))) + 1) / 2) * 2), 4, 512);
	}

	void FPainter::PathMinMax(FVec2 point)
	{
		ZoneScoped;

		m_PathMin.x = FMath::Min(point.x, m_PathMin.x);
		m_PathMin.y = FMath::Min(point.y, m_PathMin.y);

		m_PathMax.x = FMath::Max(point.x, m_PathMax.x);
		m_PathMax.y = FMath::Max(point.y, m_PathMax.y);
	}

	void FPainter::PathArcTo(const FVec2& center, float radius, float startAngle, float endAngle,
		int numSegments)
	{
		if (radius < 0.5f)
		{
			PathInsert(center);
			return;
		}

		for (int i = 0; i <= numSegments; ++i)
		{
			const float angle = startAngle + ((float)i / (float)numSegments) * (endAngle - startAngle);
			PathInsert(FVec2(center.x + FMath::Cos(angle) * radius, center.y + FMath::Sin(angle) * radius));
		}
	}

	void FPainter::PathArcToFastInternal(const FVec2& center, float radius, int sampleMin, int sampleMax, int step)
	{
		if (radius < MinCircleRadius)
		{
			PathInsert(center);
			return;
		}

		// Calculate arc auto segment step size
		if (step <= 0)
			step = kArcFastTableSize / CalculateNumCircleSegments(radius);

		// Make sure we never do steps larger than one quarter of the circle
		step = FMath::Clamp(step, 1, (int)kArcFastTableSize / 4);

		const int sampleRange = abs(sampleMax - sampleMin);
		const int nextStep = step;

		int samples = sampleRange + 1;
		bool extraMaxSample = false;
		if (step > 1)
		{
			samples = sampleRange / step + 1;
			const int overstep = sampleRange % step;

			if (overstep > 0)
			{
				extraMaxSample = true;
				samples++;

				// When we have overstep to avoid awkwardly looking one long line and one tiny one at the end,
				// distribute first step range evenly between them by reducing first step size.
				if (sampleRange > 0)
					step -= (step - overstep) / 2;
			}
		}

		m_Path.InsertRange(samples, FVec2());

		FVec2* outPtr = m_Path.GetData() + (m_Path.GetCount() - samples);

		int sampleIndex = sampleMin;
		if (sampleIndex < 0 || sampleIndex >= kArcFastTableSize)
		{
			sampleIndex = sampleIndex % kArcFastTableSize;
			if (sampleIndex < 0)
				sampleIndex += kArcFastTableSize;
		}

		if (sampleMax >= sampleMin)
		{
			for (int a = sampleMin; a <= sampleMax; a += step, sampleIndex += step, step = nextStep)
			{
				if (sampleIndex >= kArcFastTableSize)
					sampleIndex -= kArcFastTableSize;

				const FVec2 s = m_ArcFastVertex[sampleIndex];
				outPtr->x = center.x + s.x * radius;
				outPtr->y = center.y + s.y * radius;
				*outPtr = GetCurrentTransform().TransformPoint(*outPtr);
				PathMinMax(*outPtr);
				outPtr++;
			}
		}
		else
		{
			for (int a = sampleMin; a >= sampleMax; a -= step, sampleIndex -= step, step = nextStep)
			{
				if (sampleIndex < 0)
					sampleIndex += kArcFastTableSize;

				const FVec2 s = m_ArcFastVertex[sampleIndex];
				outPtr->x = center.x + s.x * radius;
				outPtr->y = center.y + s.y * radius;
				*outPtr = GetCurrentTransform().TransformPoint(*outPtr);
				PathMinMax(*outPtr);
				outPtr++;
			}
		}

		if (extraMaxSample)
		{
			int normalizedMaxSample = sampleMax % kArcFastTableSize;
			if (normalizedMaxSample < 0)
				normalizedMaxSample += kArcFastTableSize;

			const FVec2 s = m_ArcFastVertex[normalizedMaxSample];
			outPtr->x = center.x + s.x * radius;
			outPtr->y = center.y + s.y * radius;
			*outPtr = GetCurrentTransform().TransformPoint(*outPtr);
			PathMinMax(*outPtr);
			outPtr++;
		}
	}

	bool FPainter::PathStrokeInternal(bool closed)
	{
		if (!m_CurrentPen.IsValid())
			return true;

		ZoneScoped;

		FRect minMax = FRect(m_PathMin, m_PathMax);
		if (minMax.GetWidth() <= 0 || minMax.GetHeight() <= 0)
			return true;

		f32 thickness = m_CurrentPen.GetThickness();

		FColor penColor = m_CurrentPen.GetColor();
		penColor.a *= GetCurrentOpacity();

		u32 color = penColor.ToU32();

		FUIDrawItem drawItem{};
		drawItem.clipRectIndex = GetCurrentClipIndex();

		const bool hasGradient = m_CurrentPen.HasGradient();
		const bool isGradientArcLength = hasGradient && m_CurrentPen.GetGradientSpace() == EGradientSpace::ArcLength;
		const bool isGradientWorldSpace = hasGradient && m_CurrentPen.GetGradientSpace() == EGradientSpace::WorldSpace;

		if (hasGradient)
		{
			const FGradient& g = m_CurrentPen.GetGradient();
			drawItem.shaderType = EUIShaderType::LinearGradient;
			drawItem.gradientStartIndex = (int)m_DrawList->gradientStopArray.GetCount();
			drawItem.gradientStopCount = (int)g.GetStops().Size();

			for (const auto& stop : g.GetStops())
			{
				m_DrawList->gradientStopArray.Insert({ .packedColor = stop.Color.ToU32(), .position = stop.Position });
			}

			drawItem.data[0] = isGradientArcLength ? 0.0f : m_CurrentPen.GetGradient().GetAngle(); // angle=0: shader computes gradientT = uv.x directly
		}
		else
		{
			drawItem.shaderType = EUIShaderType::SolidColor;
		}

		u32 drawItemIndex = m_DrawList->AddDrawItem(drawItem);

		if (m_CurrentPen.GetStyle() == EPenStyle::Solid && closed && hasGradient)
		{
			m_Path.Insert(m_Path[0]);
		}

		// Pre-compute per-point t values (0→1 along m_Path length) for gradient pens
		m_TempPoints.RemoveAll();
		if (isGradientArcLength)
		{
			const int numPoints = (int)m_Path.GetCount();
			const int segCount = closed ? numPoints : numPoints - 1;

			f32 totalLength = 0;
			for (int i = 0; i < segCount; i++)
				totalLength += FVec2::Distance(m_Path[i], m_Path[(i + 1) % numPoints]);

			const f32 gradientOffset = m_CurrentPen.GetGradientOffset();

			m_TempPoints.InsertRange(numPoints, 0.0f);
			m_TempPoints[0] = gradientOffset;
			f32 acc = 0;

			for (int i = 0; i < segCount; i++)
			{
				const int j = (i + 1) % numPoints;
				acc += FVec2::Distance(m_Path[i], m_Path[j]);
				if (j != 0)
					m_TempPoints[j] = totalLength > 0 ? acc / totalLength + gradientOffset : 0.0f;
			}
		}

		switch (m_CurrentPen.GetStyle())
		{
		case EPenStyle::None:
			return true;
		case EPenStyle::Solid:
			// Only do closed line if we are not rendering a gradient
			m_DrawList->AddPolyLine(m_Path.GetData(), (int)m_Path.GetCount(), color, thickness, closed && !hasGradient, m_AntiAliased, drawItemIndex,
				isGradientArcLength ? m_TempPoints.GetData() : nullptr, isGradientWorldSpace ? &minMax : nullptr, isGradientWorldSpace ? m_CurrentPen.GetGradientOffset() : 0.0f);
			break;
		case EPenStyle::Dashed:
		{
			const f32 dashLen = m_CurrentPen.GetDashLength();
			const f32 gapLen = m_CurrentPen.GetDashGap();

			f32  dashOffset = 0.0f;
			bool inDash = true;
			m_CurrentPen.InitDashState(dashOffset, inDash);

			const int numPoints = (int)m_Path.GetCount();

			for (int i = 0; i < numPoints; ++i)
			{
				if (!closed && i == numPoints - 1)
					break;

				const int  nextIdx = (i + 1) % numPoints;
				const FVec2 p0 = m_Path[i];
				const FVec2 p1 = m_Path[nextIdx];
				const f32  segLen = FVec2::Distance(p0, p1);

				if (segLen < 0.001f)
					continue;

				const FVec2 dir = (p1 - p0) / segLen;
				const f32  t0 = isGradientArcLength ? m_TempPoints[i] : 0.0f;
				const f32  t1 = isGradientArcLength ? m_TempPoints[nextIdx] : 0.0f;

				f32 segPos = 0.0f;
				while (segPos < segLen - 0.001f)
				{
					const f32 remaining = segLen - segPos;

					if (inDash)
					{
						const f32 drawLen = FMath::Min(remaining, dashLen - dashOffset);
						const f32 aFrac = segPos / segLen;
						const f32 bFrac = (segPos + drawLen) / segLen;

						FVec2 pts[2] = { p0 + dir * segPos, p0 + dir * (segPos + drawLen) };
						f32  uvs[2] = { t0 + (t1 - t0) * aFrac, t0 + (t1 - t0) * bFrac };

						m_DrawList->AddPolyLine(pts, 2, color, thickness, false, m_AntiAliased, drawItemIndex,
							isGradientArcLength ? uvs : nullptr, isGradientWorldSpace ? &minMax : nullptr);

						dashOffset += drawLen;
						segPos += drawLen;

						if (dashOffset >= dashLen - 0.001f)
						{
							dashOffset = 0.0f;
							inDash = false;
						}
					}
					else
					{
						const f32 skipLen = FMath::Min(remaining, gapLen - dashOffset);
						dashOffset += skipLen;
						segPos += skipLen;

						if (dashOffset >= gapLen - 0.001f)
						{
							dashOffset = 0.0f;
							inDash = true;
						}
					}
				}
			}
		}
		break;
		case EPenStyle::Dotted:
		{
			constexpr f32 dotLen = 2.0f;
			const f32     gapLen = m_CurrentPen.GetDashGap();

			f32  dashOffset = 0.0f;
			bool inDash = true;
			m_CurrentPen.InitDashState(dashOffset, inDash);

			const int numPoints = (int)m_Path.GetCount();

			for (int i = 0; i < numPoints; ++i)
			{
				if (!closed && i == numPoints - 1)
					break;

				const int  nextIdx = (i + 1) % numPoints;
				const FVec2 p0 = m_Path[i];
				const FVec2 p1 = m_Path[nextIdx];
				const f32  segLen = FVec2::Distance(p0, p1);

				if (segLen < 0.001f)
					continue;

				const FVec2 dir = (p1 - p0) / segLen;
				const f32  t0 = hasGradient ? m_TempPoints[i] : 0.0f;
				const f32  t1 = hasGradient ? m_TempPoints[nextIdx] : 0.0f;

				f32 segPos = 0.0f;
				while (segPos < segLen - 0.001f)
				{
					const f32 remaining = segLen - segPos;

					if (inDash)
					{
						const f32 drawLen = FMath::Min(remaining, dotLen - dashOffset);
						const f32 aFrac = segPos / segLen;
						const f32 bFrac = (segPos + drawLen) / segLen;

						FVec2 pts[2] = { p0 + dir * segPos, p0 + dir * (segPos + drawLen) };
						f32  uvs[2] = { t0 + (t1 - t0) * aFrac, t0 + (t1 - t0) * bFrac };

						m_DrawList->AddPolyLine(pts, 2, color, thickness, false, m_AntiAliased, drawItemIndex,
							isGradientArcLength ? uvs : nullptr, isGradientWorldSpace ? &minMax : nullptr);

						dashOffset += drawLen;
						segPos += drawLen;

						if (dashOffset >= dotLen - 0.001f)
						{
							dashOffset = 0.0f;
							inDash = false;
						}
					}
					else
					{
						const f32 skipLen = FMath::Min(remaining, gapLen - dashOffset);
						dashOffset += skipLen;
						segPos += skipLen;

						if (dashOffset >= gapLen - 0.001f)
						{
							dashOffset = 0.0f;
							inDash = true;
						}
					}
				}
			}
		}
		break;
		}

		return true;
	}

	bool FPainter::PathFillInternal()
	{
		if (m_Path.IsEmpty() || m_CurrentBrush.GetBrushStyle() == EBrushStyle::None)
		{
			return true;
		}

		ZoneScoped;

		FRect minMax = FRect(m_PathMin, m_PathMax);
		if (minMax.GetWidth() <= 0 || minMax.GetHeight() <= 0)
			return true;

		FColor brushColor = m_CurrentBrush.GetColor();
		brushColor.a *= GetCurrentOpacity();

		u32 color = brushColor.ToU32();

		// First item will always be a SolidFill shader.
		u32 drawItemIndex = 0;

		switch (m_CurrentBrush.GetBrushStyle())
		{
		case EBrushStyle::None:
			return true;
		case EBrushStyle::SolidFill:
			{
				FUIDrawItem drawItem{};
				drawItem.clipRectIndex = GetCurrentClipIndex();
				drawItem.shaderType = EUIShaderType::SolidColor;

				drawItemIndex = m_DrawList->AddDrawItem(drawItem);
			}
		break;
		case EBrushStyle::Gradient:
			{
				FUIDrawItem drawItem{};
				drawItem.clipRectIndex = GetCurrentClipIndex();
				drawItem.gradientStartIndex = (int)m_DrawList->gradientStopArray.GetCount();
				drawItem.gradientStopCount = (int)m_CurrentBrush.GetGradient().GetStops().Size();

				switch (m_CurrentBrush.GetGradient().GetType())
				{
				case EGradientType::Linear:
					drawItem.shaderType = EUIShaderType::LinearGradient;
					break;
				case EGradientType::Radial:
					drawItem.shaderType = EUIShaderType::RadialGradient;
					break;
				case EGradientType::Conical:
					drawItem.shaderType = EUIShaderType::ConicGradient;
					break;
				}

				if (drawItem.gradientStopCount < 2)
					return true;

				for (const auto& stop : m_CurrentBrush.GetGradient().GetStops())
				{
					m_DrawList->gradientStopArray.Insert({ .packedColor = stop.Color.ToU32(), .position = stop.Position });
				}

				const FGradient& gradient = m_CurrentBrush.GetGradient();
				drawItem.data[0] = gradient.GetAngle();
				drawItem.data[1] = gradient.IsLinear() ? gradient.GetStartPoint() : gradient.GetCenter().x;
				drawItem.data[2] = gradient.IsLinear() ? gradient.GetEndPoint()   : gradient.GetCenter().y;
				if (gradient.IsRadial())
					drawItem.data[3] = gradient.GetRadius();

				drawItemIndex = m_DrawList->AddDrawItem(drawItem);
			}
		break;
		default:
			return true;
		}
		
		m_DrawList->AddConvexPolyFilled(m_Path.GetData(), (int)m_Path.GetCount(), color, m_AntiAliased, &minMax, drawItemIndex);

		return true;
	}

	void FPainter::DrawText(const FVec2& pos, const FString& text)
	{
		FFontAtlas* atlas = m_Application->GetFontAtlas().Get();

		const f32 scale = m_CurrentFont.GetPointSize() / (f32)FFontAtlas::kSdfRenderSize;

		f32 cursorX = pos.x;
		f32 cursorY = pos.y;

		FFontMetrics metrics = atlas->GetScaledMetrics(m_CurrentFont);

		u32 drawItemIndex = m_DrawList->AddDrawItem({
			.shaderType = EUIShaderType::SDFText,
			.textureIndex = 0,
			.samplerIndex = 0,
			.drawItemFlags = EUIDrawItemFlags::None,
			.clipRectIndex = GetCurrentClipIndex(),
			.gradientStartIndex = 0,
			.gradientStopCount = 0,
			.userFlags = 0,
			.data = {}
		});

		for (char32_t codepoint : text.Codepoints())
		{
			FGlyph glyph = atlas->FindOrAddGlyph(m_CurrentFont, codepoint);
			if (!glyph.IsValid())
				continue;

			f32 quadW = glyph.Width * scale;
			f32 quadH = glyph.Height * scale;

			f32 quadX = cursorX + glyph.BearingX * scale;
			f32 quadY = cursorY - glyph.BearingY * scale;  // cursorY is the baseline

			f32 u0 = (f32)glyph.X / glyph.AtlasSize;
			f32 v0 = (f32)glyph.Y / glyph.AtlasSize;
			f32 u1 = (f32)(glyph.X + glyph.Width) / glyph.AtlasSize;
			f32 v1 = (f32)(glyph.Y + glyph.Height) / glyph.AtlasSize;

			cursorX += (f32)glyph.Advance * scale;
		}
	}

} // namespace Fusion
