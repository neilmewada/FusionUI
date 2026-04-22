#include "Fusion/Widgets.h"

namespace Fusion
{
	FExpanderBox::FExpanderBox()
	{
		m_ExpandedAmount = 1.0f;
	}

	void FExpanderBox::Construct()
	{
		Super::Construct();

		StyleScopeBoundary(true);

		PropagatedStyleStates(EStyleState::Expanded);

		Header(
			FNew(FButton)
			.PropagatedStyleStates(EStyleState::Expanded)
			.HAlign(EHAlign::Fill)
			.Child(
				FNew(FHorizontalStack)
				.PropagatedStyleStates(EStyleState::Expanded)
				.Spacing(10)
				.ContentVAlign(EVAlign::Center)
				.HAlign(EHAlign::Fill)
				.VAlign(EVAlign::Fill)
				(
					FNew(FDecoratedBox)
					.Background(FBrush::Image("embed:/Icons/CaretDown.png"))
					.SubStyle("Chevron")
					.Width(10)
					.Height(10),

					FAssignNew(FLabel, m_TitleLabel)
					.Text("Title")
				)
			)
		);

		Content(
			FNew(FDecoratedBox)
			.SubStyle("ContentBox")
			.HAlign(EHAlign::Fill)
			.FillRatio(0.0f)
		);
	}

    FVec2 FExpanderBox::MeasureContent(FVec2 availableSize)
    {
        ZoneScoped;

        FVec2 contentAvailable = FVec2(
            FMath::Max(0.0f, availableSize.x - m_Padding.left - m_Padding.right),
            FMath::Max(0.0f, availableSize.y - m_Padding.top - m_Padding.bottom)
        );

        f32 totalHeight = 0.0f;
        f32 maxWidth = 0.0f;

		if (m_Header && !m_Header->Excluded())
		{
			FMargin margin = m_Header->Margin();

			FVec2 childAvailable = FVec2(
                FMath::Max(0.0f, contentAvailable.x - margin.left - margin.right),
                FMath::Max(0.0f, contentAvailable.y - margin.top - margin.bottom)
            );

			FVec2 childDesired = FVec2();

			FUSION_TRY
			{
				childDesired = m_Header->MeasureContent(childAvailable);
			}
			FUSION_CATCH(const FException & exception)
			{
				FUSION_LOG_ERROR("Widget", "Exception: {}. Exception thrown by a Widget [{}] in FExpanderBox::MeasureContent.\n{}",
					exception.what(), m_Header->GetClassName(), exception.GetStackTraceString(true));

				SetFaulted();
			}

			totalHeight += childDesired.y + margin.top + margin.bottom;
			maxWidth = FMath::Max(maxWidth, childDesired.x + margin.left + margin.right);
		}

		if (m_Content && !m_Content->Excluded() && m_ExpandedAmount > 0.0001f)
		{
			FMargin margin = m_Content->Margin();

			FVec2 childAvailable = FVec2(
				FMath::Max(0.0f, contentAvailable.x - margin.left - margin.right),
				FMath::Max(0.0f, contentAvailable.y - margin.top - margin.bottom)
			);

			FVec2 childDesired = FVec2();

			FUSION_TRY
			{
				childDesired = m_Content->MeasureContent(childAvailable);
			}
			FUSION_CATCH(const FException & exception)
			{
				FUSION_LOG_ERROR("Widget", "Exception: {}. Exception thrown by a Widget [{}] in FExpanderBox::MeasureContent.\n{}",
					exception.what(), m_Content->GetClassName(), exception.GetStackTraceString(true));

				SetFaulted();
			}

			totalHeight += childDesired.y + margin.top + margin.bottom;
			maxWidth = FMath::Max(maxWidth, childDesired.x + margin.left + margin.right);
		}

        FVec2 desired = FVec2(
            maxWidth + m_Padding.left + m_Padding.right,
            totalHeight + m_Padding.top + m_Padding.bottom
        );

        return m_DesiredSize = ApplyLayoutConstraints(desired);
    }

    void FExpanderBox::ArrangeContent(FVec2 finalSize)
    {
        ZoneScoped;

        Super::ArrangeContent(finalSize);

        f32 contentWidth = FMath::Max(0.0f, GetLayoutSize().x - m_Padding.left - m_Padding.right);
        f32 contentHeight = FMath::Max(0.0f, GetLayoutSize().y - m_Padding.top - m_Padding.bottom);

        f32 contentMain = contentHeight;
        f32 contentCross = contentWidth;

        // ── Pass 1: fixed main-axis budget + fill-child collection ───────────
        f32 fixedMain = 0.0f;

        struct FillEntry
        {
            int  childIdx;
            f32  ratio;
            f32  allocatedMain; // resolved main-axis size (post-clamping)
            bool frozen;        // true once clamped by MaxWidth/MaxHeight
        };

        // Collect fill children in iteration order (indices into children array)
        FArray<FillEntry> fillEntries;
        fillEntries.Reserve(GetChildCount());

        for (int i = 0; i < (int)GetChildCount(); i++)
        {
            Ref<FWidget> child = GetChildAt(i);
            if (child->Excluded()) continue;

            FMargin m = child->Margin();
            f32 fillRatio = child->FillRatio();
            f32 mainMargin = (m.top + m.bottom);

            if (fillRatio > 0.0f)
            {
                fillEntries.Add({ i, fillRatio, 0.0f, false });
                fixedMain += mainMargin; // only margin is fixed; size comes from the pool
            }
            else
            {
                FVec2 d = child->GetDesiredSize();
                fixedMain += (d.y + mainMargin);
            }
        }

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
                FVec2 probe = FVec2(contentCross, allocated);
                FVec2 clamped = child->ApplyLayoutConstraints(probe);
                f32  clampedMain = clamped.y;

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
        f32  cursor = m_Padding.top;
        bool isFirst = true;
        int  fillSlot = 0; // walking index into fillEntries (same order as children)

        for (int i = 0; i < GetChildCount(); i++)
        {
            Ref<FWidget> child = GetChildAt(i);
            if (child->Excluded()) continue;

            isFirst = false;

            FMargin childMargin = child->Margin();
            FVec2   childDesired = child->GetDesiredSize();
            f32     fillRatio = child->FillRatio();

            // ── Main axis ────────────────────────────────────────────────────
            f32 mainMarginStart = childMargin.top;
            f32 mainMarginEnd = childMargin.bottom;

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
                childMainSize = childDesired.y;
            }

            f32 childMainPos = cursor + mainMarginStart;

            // ── Cross axis ───────────────────────────────────────────────────
            f32 crossMarginStart = childMargin.left;
            f32 crossMarginEnd = childMargin.right;
            f32 crossDesired = childDesired.x;
            f32 crossAreaSize = FMath::Max(0.0f, contentCross - crossMarginStart - crossMarginEnd);
            f32 crossPaddingStart = m_Padding.left;

            f32 childCrossSize;
            f32 childCrossPos;

            EHAlign hAlign = child->HAlign();

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

            FUSION_TRY
            {
                // Arrange and advance the cursor using the child's actual layout size
                // (post-constraint) rather than our pre-computed estimate.
                child->SetLayoutPosition(FVec2(childCrossPos, childMainPos));
                child->ArrangeContent(FVec2(childCrossSize, childMainSize));
                cursor += mainMarginStart + child->GetLayoutSize().y + mainMarginEnd;
            }
            FUSION_CATCH(const FException & exception)
            {
                FUSION_LOG_ERROR("Widget", "Exception: {}. Exception thrown by a Widget [{}] in FStackBox::ArrangeContent.\n{}",
                    exception.what(), child->GetClassName(), exception.GetStackTraceString(true));

                SetFaulted();
            }
        }
    }

	void FExpanderBox::Paint(FPainter& painter)
	{
		Super::Paint(painter);

		FVec2 layoutSize = GetLayoutSize();
		FRect widgetRect(0, 0, layoutSize.width, layoutSize.height);

		painter.SetBrush(Background());
		painter.SetPen(Border());
		painter.FillAndStrokeShape(widgetRect, Shape());
	}

	void FExpanderBox::SetExpanded(bool expanded)
	{
		if (TestStyleState(EStyleState::Expanded) == expanded)
			return;

		SetStyleStateFlag(EStyleState::Expanded, expanded);
	}

	void FExpanderBox::SetupHeader()
	{
		if (!m_Header)
			return;

		(*m_Header)
		.PropagatedStyleStates(EStyleState::Expanded)
		.SubStyle("Header")
		.OnClick([this]
		{
			SetExpanded(!IsExpanded());
		});
	}

	void FExpanderBox::SetupContent()
	{
		if (m_Content)
		{
            (*m_Content)
            .SubStyle("Content");
		}
	}

	void FExpanderBox::OnSlotSet(const FName& slotName)
	{
		Super::OnSlotSet(slotName);

        thread_local const FName headerSlot = "Header";
        if (slotName == headerSlot)
        {
            SetupHeader();
        }
	}
} // namespace Fusion
