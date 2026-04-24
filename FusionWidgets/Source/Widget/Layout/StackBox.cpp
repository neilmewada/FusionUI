#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	FStackBox::FStackBox()
	{
		m_HAlign = EHAlign::Fill;
		m_VAlign = EVAlign::Fill;
	}

	void FStackBox::SetParentSurfaceRecursive(Ref<FSurface> surface)
	{
		Super::SetParentSurfaceRecursive(surface);

		for (u32 i = 0; i < GetChildCount(); i++)
		{
			GetChildAt(i)->SetParentSurfaceRecursive(surface);
		}
	}

	FVec2 FStackBox::MeasureContent(FVec2 availableSize)
	{
        ZoneScoped;

        FVec2 contentAvailable = FVec2(
            FMath::Max(0.0f, availableSize.x - m_Padding.left - m_Padding.right),
            FMath::Max(0.0f, availableSize.y - m_Padding.top - m_Padding.bottom)
        );

        f32 totalMainAxis = 0.0f;
        f32 maxCrossAxis = 0.0f;
        int enabledCount = 0;

        for (u32 i = 0; i < GetChildCount(); i++)
        {
            Ref<FWidget> child = GetChildAt(i);
            if (child->Excluded())
                continue;

            FMargin childMargin = child->Margin();

            FVec2 childAvailable = FVec2(
                FMath::Max(0.0f, contentAvailable.x - childMargin.left - childMargin.right),
                FMath::Max(0.0f, contentAvailable.y - childMargin.top - childMargin.bottom)
            );

            FVec2 childDesired = FVec2();

            FUSION_TRY
            {
                childDesired = child->MeasureContent(childAvailable);
            }
            FUSION_CATCH(const FException& exception)
            {
                FUSION_LOG_ERROR("Widget", "Exception: {}. Exception thrown by a Widget [{}] in FStackBox::MeasureContent.\n{}", 
                    exception.what(), child->GetClassName(), exception.GetStackTraceString(true));

                child->SetFaulted();
            }

            if (m_Direction == EStackDirection::Horizontal)
            {
                totalMainAxis += childDesired.x + childMargin.left + childMargin.right;
                maxCrossAxis = FMath::Max(maxCrossAxis, childDesired.y + childMargin.top + childMargin.bottom);
            }
            else
            {
                totalMainAxis += childDesired.y + childMargin.top + childMargin.bottom;
                maxCrossAxis = FMath::Max(maxCrossAxis, childDesired.x + childMargin.left + childMargin.right);
            }

            enabledCount++;
        }

        if (enabledCount > 1)
        {
            totalMainAxis += (enabledCount - 1) * m_Spacing;
        }

        FVec2 desired;
        if (m_Direction == EStackDirection::Horizontal)
        {
            desired = FVec2(
                totalMainAxis + m_Padding.left + m_Padding.right,
                maxCrossAxis + m_Padding.top + m_Padding.bottom
            );
        }
        else
        {
            desired = FVec2(
                maxCrossAxis + m_Padding.left + m_Padding.right,
                totalMainAxis + m_Padding.top + m_Padding.bottom
            );
        }

        return m_DesiredSize = ApplyLayoutConstraints(desired);
	}

	void FStackBox::ArrangeContent(FVec2 finalSize)
	{
        ZoneScoped;

        Super::ArrangeContent(finalSize);

        bool isHorizontal = (m_Direction == EStackDirection::Horizontal);

        f32 contentWidth = FMath::Max(0.0f, GetLayoutSize().x - m_Padding.left - m_Padding.right);
        f32 contentHeight = FMath::Max(0.0f, GetLayoutSize().y - m_Padding.top - m_Padding.bottom);
        f32 contentMain = isHorizontal ? contentWidth : contentHeight;
        f32 contentCross = isHorizontal ? contentHeight : contentWidth;

        // ── Pass 1: fixed main-axis budget + fill-child collection ───────────
        int enabledCount = 0;
        f32 fixedMain = 0.0f;

        struct FillEntry
        {
            int  childIdx;
            f32  ratio;
            f32  allocatedMain; // resolved main-axis size (post-clamping)
            bool frozen;        // true once clamped by MaxWidth/MaxHeight
        };

        // Collect fill children in iteration order (indices into children array)
        TArray<FillEntry> fillEntries;
        fillEntries.Reserve(GetChildCount());

        for (int i = 0; i < (int)GetChildCount(); i++)
        {
            Ref<FWidget> child = GetChildAt(i);
            if (child->Excluded()) continue;

            FMargin m = child->Margin();
            f32 fillRatio = child->FillRatio();
            f32 mainMargin = isHorizontal ? (m.left + m.right) : (m.top + m.bottom);

            if (fillRatio > 0.0f)
            {
                fillEntries.Add({ i, fillRatio, 0.0f, false });
                fixedMain += mainMargin; // only margin is fixed; size comes from the pool
            }
            else
            {
                FVec2 d = child->GetDesiredSize();
                fixedMain += isHorizontal ? (d.x + mainMargin) : (d.y + mainMargin);
            }

            enabledCount++;
        }

        if (enabledCount > 1)
            fixedMain += (enabledCount - 1) * m_Spacing;

        // ── Iterative fill distribution (CSS flex-grow semantics) ────────────
        // Each iteration freezes any fill child whose proportional share exceeds
        // its MaxWidth/MaxHeight, returns the excess to the pool, and repeats
        // until no new child is frozen.  This mirrors the CSS flexbox algorithm.
        f32 availableForFill = FMath::Max(0.0f, contentMain - fixedMain);

        bool anyFrozenThisRound = true;
        while (anyFrozenThisRound && fillEntries.Size() > 0)
        {
            anyFrozenThisRound = false;

            f32 sumRatios = 0.0f;
            for (int e = 0; e < fillEntries.Size(); e++)
                if (!fillEntries[e].frozen) sumRatios += fillEntries[e].ratio;

            if (sumRatios <= 0.0f) break;

            for (int e = 0; e < fillEntries.Size(); e++)
            {
                FillEntry& entry = fillEntries[e];
                if (entry.frozen) continue;

                f32 allocated = availableForFill * (entry.ratio / sumRatios);

                // Check whether this child's constraints clamp the main axis.
                // We probe with the full contentCross for the cross dimension;
                // only the main-axis result of ApplyLayoutConstraints matters here.
                Ref<FWidget> child = GetChildAt(entry.childIdx);
                FVec2 probe = isHorizontal ? FVec2(allocated, contentCross)
                    : FVec2(contentCross, allocated);
                FVec2 clamped = child->ApplyLayoutConstraints(probe);
                f32  clampedMain = isHorizontal ? clamped.x : clamped.y;

                if (clampedMain < allocated - 0.001f) // clamped — freeze at max
                {
                    entry.allocatedMain = clampedMain;
                    entry.frozen = true;
                    availableForFill = FMath::Max(0.0f, availableForFill - clampedMain);
                    anyFrozenThisRound = true;
                }
            }
        }

        // Distribute whatever is left to the still-unfrozen fill children
        {
            f32 sumRatios = 0.0f;
            for (int e = 0; e < fillEntries.Size(); e++)
                if (!fillEntries[e].frozen) sumRatios += fillEntries[e].ratio;

            for (int e = 0; e < fillEntries.Size(); e++)
            {
                FillEntry& entry = fillEntries[e];
                if (!entry.frozen)
                    entry.allocatedMain = (sumRatios > 0.0f)
                    ? FMath::Max(0.0f, availableForFill * (entry.ratio / sumRatios))
                    : 0.0f;
            }
        }

        // ── Pass 2: position and arrange each child ──────────────────────────
        f32  cursor = isHorizontal ? m_Padding.left : m_Padding.top;
        bool isFirst = true;
        int  fillSlot = 0; // walking index into fillEntries (same order as children)

        for (int i = 0; i < (int)GetChildCount(); i++)
        {
            Ref<FWidget> child = GetChildAt(i);
            if (child->Excluded()) continue;

            if (!isFirst) cursor += m_Spacing;
            isFirst = false;

            FMargin childMargin = child->Margin();
            FVec2   childDesired = child->GetDesiredSize();
            f32     fillRatio = child->FillRatio();

            // ── Main axis ────────────────────────────────────────────────────
            f32 mainMarginStart = isHorizontal ? childMargin.left : childMargin.top;
            f32 mainMarginEnd = isHorizontal ? childMargin.right : childMargin.bottom;

            f32 childMainSize;
            if (fillRatio > 0.0f)
            {
                // Advance the fill slot to match this child index
                while (fillSlot < fillEntries.Size() && fillEntries[fillSlot].childIdx < i)
                    fillSlot++;
                childMainSize = (fillSlot < fillEntries.Size() && fillEntries[fillSlot].childIdx == i)
                    ? fillEntries[fillSlot].allocatedMain : 0.0f;
            }
            else
            {
                childMainSize = isHorizontal ? childDesired.x : childDesired.y;
            }

            f32 childMainPos = cursor + mainMarginStart;

            // ── Cross axis ───────────────────────────────────────────────────
            f32 crossMarginStart = isHorizontal ? childMargin.top : childMargin.left;
            f32 crossMarginEnd = isHorizontal ? childMargin.bottom : childMargin.right;
            f32 crossDesired = isHorizontal ? childDesired.y : childDesired.x;
            f32 crossAreaSize = FMath::Max(0.0f, contentCross - crossMarginStart - crossMarginEnd);
            f32 crossPaddingStart = isHorizontal ? m_Padding.top : m_Padding.left;

            f32 childCrossSize;
            f32 childCrossPos;

            if (isHorizontal)
            {
                EVAlign vAlign = child->VAlign();
                if (vAlign == EVAlign::Auto) vAlign = m_ContentVAlign;

                switch (vAlign)
                {
                default:
                case EVAlign::Auto:
                case EVAlign::Fill:
                    childCrossSize = crossAreaSize;
                    childCrossPos = crossPaddingStart + crossMarginStart;
                    break;
                case EVAlign::Top:
                    childCrossSize = FMath::Min(crossDesired, crossAreaSize);
                    childCrossPos = crossPaddingStart + crossMarginStart;
                    break;
                case EVAlign::Center:
                    childCrossSize = FMath::Min(crossDesired, crossAreaSize);
                    childCrossPos = crossPaddingStart + crossMarginStart + (crossAreaSize - childCrossSize) / 2.0f;
                    break;
                case EVAlign::Bottom:
                    childCrossSize = FMath::Min(crossDesired, crossAreaSize);
                    childCrossPos = crossPaddingStart + crossMarginStart + (crossAreaSize - childCrossSize);
                    break;
                }
            }
            else
            {
                EHAlign hAlign = child->HAlign();
                if (hAlign == EHAlign::Auto) hAlign = m_ContentHAlign;

                switch (hAlign)
                {
                default:
                case EHAlign::Auto:
                case EHAlign::Fill:
                    childCrossSize = crossAreaSize;
                    childCrossPos = crossPaddingStart + crossMarginStart;
                    break;
                case EHAlign::Left:
                    childCrossSize = FMath::Min(crossDesired, crossAreaSize);
                    childCrossPos = crossPaddingStart + crossMarginStart;
                    break;
                case EHAlign::Center:
                    childCrossSize = FMath::Min(crossDesired, crossAreaSize);
                    childCrossPos = crossPaddingStart + crossMarginStart + (crossAreaSize - childCrossSize) / 2.0f;
                    break;
                case EHAlign::Right:
                    childCrossSize = FMath::Min(crossDesired, crossAreaSize);
                    childCrossPos = crossPaddingStart + crossMarginStart + (crossAreaSize - childCrossSize);
                    break;
                }
            }

            FUSION_TRY
            {
                // Arrange and advance the cursor using the child's actual layout size
                // (post-constraint) rather than our pre-computed estimate.
                if (isHorizontal)
                {
                    child->SetLayoutPosition(FVec2(childMainPos, childCrossPos));
                    child->ArrangeContent(FVec2(childMainSize, childCrossSize));
                    cursor += mainMarginStart + child->GetLayoutSize().x + mainMarginEnd;
                }
                else
                {
                    child->SetLayoutPosition(FVec2(childCrossPos, childMainPos));
                    child->ArrangeContent(FVec2(childCrossSize, childMainSize));
                    cursor += mainMarginStart + child->GetLayoutSize().y + mainMarginEnd;
                }
            }
            FUSION_CATCH(const FException& exception)
            {
                FUSION_LOG_ERROR("Widget", "Exception: {}. Exception thrown by a Widget [{}] in FStackBox::ArrangeContent.\n{}",
                    exception.what(), child->GetClassName(), exception.GetStackTraceString(true));

                child->SetFaulted();
            }
        }
	}

	void FVerticalStack::OnPropertyModified(const FName& propertyName)
	{
        Super::OnPropertyModified(propertyName);

        thread_local const FName stackDirectionProperty = "Direction";

        if (propertyName == stackDirectionProperty)
        {
            m_Direction = EStackDirection::Vertical;
        }
	}

	void FHorizontalStack::OnPropertyModified(const FName& propertyName)
	{
        Super::OnPropertyModified(propertyName);

        thread_local const FName stackDirectionProperty = "Direction";

        if (propertyName == stackDirectionProperty)
        {
            m_Direction = EStackDirection::Horizontal;
        }
	}
} // namespace Fusion
