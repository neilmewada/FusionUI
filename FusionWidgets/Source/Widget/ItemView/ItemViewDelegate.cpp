#include "Fusion/Widgets.h"

namespace Fusion
{
    void FItemViewDelegate::Paint(FPainter& painter, FModelIndex index, const FItemViewPaintInfo& info)
    {
        const f32   leftPadding   = info.View->RowLeftPadding();
        const f32   chevronSize   = info.View->RowChevronSize();
        const f32   chevronGap    = info.View->RowChevronGap();
        const f32   iconGap       = info.View->RowIconGap();
        const bool  expandable    = index.Column() == 0 && info.View->IsExpandable();
        const u32   childrenCount = info.Model->GetRowCount(index);
        const bool  showExpander  = expandable && childrenCount > 0;
        const f32   iconWidth     = info.View->RowIconWidth();
        const bool  hasIcons      = iconWidth > 0.001f && info.Model->HasIcons(index.Column());
        const FVec2 rectSize      = info.Rect.GetSize();
        const f32   centerY       = info.Rect.min.y + rectSize.height * 0.5f;
        const bool  isExpanded    = info.View->IsExpanded(index);

        painter.PushClip(info.Rect, FRectangle());

        // Cursor advances left-to-right as elements are placed
        f32 cursorX = info.Rect.min.x + leftPadding;

        // --- Chevron ---
        // Always reserve chevron space for expandable columns so text
        // aligns consistently across rows with and without children.
        if (expandable)
        {
            if (showExpander)
            {
                thread_local const FName caretRight = "embed:/Icons/CaretRight.png";
                thread_local const FName caretDown = "embed:/Icons/CaretDown.png";

                FBrush chevronBrush = FBrush::Image(isExpanded ? caretDown : caretRight)
                    .BrushSize(FVec2(chevronSize, chevronSize));

                painter.SetBrush(chevronBrush);
                painter.FillRect(FRect::FromSize(
                    FVec2(cursorX, centerY - chevronSize * 0.5f),
                    FVec2(chevronSize, chevronSize)));
            }
            cursorX += chevronSize + chevronGap;
        }

        // --- Icon ---
        // Always reserve icon space when the column has icons so text
        // aligns consistently across rows with and without an icon.
        if (hasIcons)
        {
            FVariant icon = info.Model->GetItemData(index, EItemRole::Icon);
            FString  iconPath;

            if (icon.Has<FName>())
                iconPath = icon.Get<FName>().ToString();
            else if (icon.Has<FString>())
                iconPath = icon.Get<FString>();

            if (!iconPath.Empty())
            {
                FBrush iconBrush = FBrush::Image(iconPath)
                    .BrushSize(FVec2(iconWidth, iconWidth));

                painter.SetBrush(iconBrush);
                painter.FillRect(FRect::FromSize(
                    FVec2(cursorX, centerY - iconWidth * 0.5f),
                    FVec2(iconWidth, iconWidth)));
            }
            cursorX += iconWidth + iconGap;
        }

        // --- Text ---
        FVariant content = info.Model->GetItemData(index, EItemRole::Content);

        if (content.Has<FString>())
        {
            painter.SetFont(FFont::Regular(FFont::kDefaultFamilyName, 12.0f));
            painter.SetPen(FColors::White);

            FRect textRect = FRect(FVec2(cursorX, info.Rect.min.y), info.Rect.max);
            painter.DrawText(textRect, content.Get<FString>(),
                ETextWrap::None, EHAlign::Left, EVAlign::Center);
        }

        painter.PopClip();
    }
} // namespace Fusion
