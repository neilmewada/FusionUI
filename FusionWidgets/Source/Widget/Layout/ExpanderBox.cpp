#include "Fusion/Widgets.h"

namespace Fusion
{
	FExpanderBox::FExpanderBox()
	{
		m_ExpandedAmount = 0.0f;
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

					FNew(FLabel)
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

		m_Content->Excluded(ExpandedAmount() <= kExpansionThreshold);

		if (m_Content && !m_Content->Excluded() && ExpandedAmount() > kExpansionThreshold)
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

			totalHeight += (childDesired.y + margin.top + margin.bottom) * FMath::Clamp01(ExpandedAmount());
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

        const f32 contentWidth = FMath::Max(0.0f, GetLayoutSize().x - m_Padding.left - m_Padding.right);
        const f32 contentHeight = FMath::Max(0.0f, GetLayoutSize().y - m_Padding.top - m_Padding.bottom);

		f32 cursorY = 0;

		if (m_Header && !m_Header->Excluded())
		{
			FUSION_TRY
			{
				m_Header->SetLayoutPosition(FVec2());
				m_Header->ArrangeContent(FVec2(contentWidth, m_Header->GetDesiredSize().height));
			}
			FUSION_CATCH(const FException& exception)
			{
				FUSION_LOG_ERROR("Widget", "Exception: {}. Exception thrown by a Widget [{}] in FExpanderBox::ArrangeContent on Header.\n{}",
					exception.what(), m_Header->GetClassName(), exception.GetStackTraceString(true));

				SetFaulted();
				return;
			}

			cursorY += m_Header->GetLayoutSize().height;
		}

		if (m_Content && !m_Content->Excluded() && ExpandedAmount() > kExpansionThreshold)
		{
			FUSION_TRY
			{
				m_Content->SetLayoutPosition(FVec2(0, cursorY));
				m_Content->ArrangeContent(FVec2(contentWidth, m_Content->GetDesiredSize().height * FMath::Clamp01(ExpandedAmount())));
			}
			FUSION_CATCH(const FException& exception)
			{
				FUSION_LOG_ERROR("Widget", "Exception: {}. Exception thrown by a Widget [{}] in FExpanderBox::ArrangeContent on Content.\n{}",
					exception.what(), m_Header->GetClassName(), exception.GetStackTraceString(true));

				SetFaulted();
				return;
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

		m_TitleLabel = CastTo<FLabel>(m_Header->FindSubWidgetTypeInHierarchy(FLabel::StaticClassTypeID()));
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
