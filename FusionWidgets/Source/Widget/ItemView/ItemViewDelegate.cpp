#include "Fusion/Widgets.h"

namespace Fusion
{
    void FItemViewDelegate::Paint(FPainter& painter, FModelIndex index, const FItemViewPaintInfo& info)
    {
        constexpr f32 kIconSpacing = 1.0f;

        const bool expandable = index.Column() == 0 && info.View->IsExpandable();
        const u32 childrenCount = info.Model->GetRowCount(index);
        const bool showExpander = expandable && childrenCount > 0;
        const f32 iconWidth = info.View->IconWidth();
        const bool hasIcons = iconWidth > kIconSpacing;
        const FVec2 rectSize = info.Rect.GetSize();

        f32 textOffsetX = expandable ? 10.0f : 0.0f;
        if (hasIcons)
            textOffsetX += iconWidth + kIconSpacing;

        painter.PushClip(info.Rect, FRectangle());

        thread_local const FName caretRight = "embed:/Icons/CaretRight.png";

        if (showExpander)
        {
            FBrush iconBrush = FBrush::Image(caretRight)
            .BrushSize(FVec2(1, 1) * iconWidth)
            .ImageFit(EImageFit::Auto);

            painter.SetBrush(iconBrush);

            painter.FillRect(FRect::FromSize(
                info.Rect.Translate(FVec2(kIconSpacing, (rectSize.height - iconWidth) * 0.5f)).min,
                FVec2(iconWidth, iconWidth)));
        }

        if (hasIcons)
        {
            FVariant icon = info.Model->GetItemData(index, EItemRole::Icon);
            FString iconPath = "";
            if (icon.Has<FName>())
            {
                iconPath = icon.Get<FName>().ToString();
            }
            else if (icon.Has<FString>())
            {
                iconPath = icon.Get<FString>();
            }

            if (!iconPath.Empty())
            {

            }
        }

        FVariant content = info.Model->GetItemData(index, EItemRole::Content);

        if (content.Has<FString>())
        {
            painter.SetFont(FFont::Regular(FFont::kDefaultFamilyName, 12.0f));
            painter.SetPen(FColors::White);

            const FString text = content.Get<FString>();
            painter.DrawText(info.Rect.Translate(FVec2(textOffsetX, 0)),
                text, ETextWrap::None, EHAlign::Left, EVAlign::Center);
        }

        painter.PopClip();
    }
} // namespace Fusion
