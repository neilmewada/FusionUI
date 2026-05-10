#include "Fusion/Widgets.h"

namespace Fusion
{
    FItemViewLayout FItemViewDelegate::Paint(FPainter& painter, FModelIndex index, const FItemViewCellInfo& info)
    {
        Ref<FItemModel> model = index.GetModel();
        if (!model)
            return {};

        const f32   leftPadding   = info.LeftPadding;
        const f32   chevronSize   = info.ChevronSize;
        const f32   chevronGap    = info.ChevronGap;
        const f32   iconGap       = info.IconGap;
        const bool  expandable    = index.Column() == 0 && info.IsExpandable;
        const u32   childrenCount = model->GetRowCount(index);
        const bool  showExpander  = expandable && childrenCount > 0;
        const f32   iconWidth     = info.IconWidth;
        const bool  hasIcons      = iconWidth > 0.001f && model->HasIcons(index.Column());
        const FVec2 rectSize      = info.Rect.GetSize();
        const f32   centerY       = info.Rect.top + rectSize.height * 0.5f;
        const bool  isExpanded    = info.IsExpanded;

        painter.PushClip(info.Rect, FRectangle());

        // Cursor advances left-to-right as elements are placed
        f32 cursorX = info.Rect.left + leftPadding;

        FItemViewLayout result{};

        // --- Chevron ---
        // Always reserve chevron space for expandable columns so text
        // aligns consistently across rows with and without children.
        if (expandable)
        {
            if (showExpander)
            {
                thread_local const FName caretRight = "embed:/Icons/CaretRight.png";
                thread_local const FName caretDown = "embed:/Icons/CaretDown.png";

                FBrush chevronBrush = FBrush::Image(isExpanded ? caretDown : caretRight, info.ChevronColor)
                    .BrushSize(FVec2(chevronSize, chevronSize));

                result.ChevronRect = FRect::FromSize(
                    FVec2(cursorX, centerY - chevronSize * 0.5f),
                    FVec2(chevronSize, chevronSize));

                painter.SetBrush(chevronBrush);
                painter.FillRect(result.ChevronRect);
            }
            cursorX += chevronSize + chevronGap;
        }

        // --- Icon ---
        // Always reserve icon space when the column has icons so text
        // aligns consistently across rows with and without an icon.
        if (hasIcons)
        {
            FVariant icon = model->GetItemData(index, EItemRole::Icon);
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
        FVariant content = model->GetItemData(index, EItemRole::Content);

        if (content.Has<FString>())
        {
            painter.SetFont(FFont::Regular(FFont::kDefaultFamilyName, 12.0f));
            painter.SetPen(FColors::White);

            FRect textRect = FRect(FVec2(cursorX, info.Rect.top), FVec2(info.Rect.right, info.Rect.bottom));
            painter.DrawText(textRect, content.Get<FString>(),
                ETextWrap::None, EHAlign::Left, EVAlign::Center);
        }

        painter.PopClip();

        return result;
    }

} // namespace Fusion
