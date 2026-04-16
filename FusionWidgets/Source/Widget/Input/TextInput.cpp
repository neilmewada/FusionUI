#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    // -----------------------------------------------------------------------
    // UTF-8 / codepoint helpers
    // -----------------------------------------------------------------------

    const char* FTextInput::Utf8Advance(const char* ptr)
    {
        auto b = static_cast<uint8_t>(*ptr);
        if      (b < 0x80) return ptr + 1;
        else if (b < 0xE0) return ptr + 2;
        else if (b < 0xF0) return ptr + 3;
        else               return ptr + 4;
    }

    int FTextInput::CpToByteOffset(const char* str, int cpIndex)
    {
        const char* ptr = str;
        for (int i = 0; i < cpIndex; i++)
            ptr = Utf8Advance(ptr);
        return (int)(ptr - str);
    }

    int FTextInput::CpCount(const FString& str)
    {
        int count = 0;
        for ([[maybe_unused]] char32_t cp : str.Codepoints())
            count++;
        return count;
    }

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    FTextInput::FTextInput()
    {
        m_ClipContent = true;
    }

    // -----------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------

    FVec2 FTextInput::MeasureContent(FVec2 availableSize)
    {
        f32 height = 32.0f; // fallback

        if (Ref<FApplicationInstance> app = GetApplication())
        {
            FFontAtlas* atlas = app->GetFontAtlas().Get();
            FFontMetrics metrics = atlas->GetScaledMetrics(Font());
            FMargin pad = Padding();
            height = metrics.LineHeight + pad.top + pad.bottom;
        }

        return m_DesiredSize = ApplyLayoutConstraints(FVec2(availableSize.x, height));
    }

    void FTextInput::ArrangeContent(FVec2 finalSize)
    {
        Super::ArrangeContent(finalSize);
    }

    // -----------------------------------------------------------------------
    // Text helpers
    // -----------------------------------------------------------------------

    FString FTextInput::GetDisplayText() const
    {
        if (!IsPassword() || m_Text.Empty())
            return m_Text;

        // Replace every codepoint with U+2022 BULLET '•' (UTF-8: E2 80 A2)
        std::string result;
        result.reserve(m_Text.ByteLength()); // approximate
        for ([[maybe_unused]] char32_t cp : m_Text.Codepoints())
            result += "\xE2\x80\xA2";
        return FString(result);
    }

    f32 FTextInput::MeasureTextWidth(const char* start, const char* end) const
    {
        if (start >= end)
            return 0.0f;

        Ref<FApplicationInstance> app = GetApplication();
        if (!app)
            return 0.0f;

        FFontAtlas* atlas = app->GetFontAtlas().Get();
        FFont font = Font();
        const f32 scale = font.GetPointSize() / (f32)FFontAtlas::kSdfRenderSize;

        f32 width = 0.0f;
        FString::CodepointIterator it{ start };
        FString::CodepointIterator itEnd{ end };

        while (it != itEnd)
        {
            char32_t cp = *it;
            ++it;
            FGlyph glyph = atlas->FindOrAddGlyph(font, cp);
            if (glyph.IsValid())
                width += glyph.Advance * scale;
        }
        return width;
    }

    f32 FTextInput::CursorPixelX(int cpIndex) const
    {
        FString display = GetDisplayText();
        const char* start = display.CStr();

        // Advance cpIndex codepoints into the display string
        const char* ptr = start;
        for (int i = 0; i < cpIndex; i++)
            ptr = Utf8Advance(ptr);

        return MeasureTextWidth(start, ptr);
    }

    int FTextInput::HitTestCursorIndex(f32 localX) const
    {
        Ref<FApplicationInstance> app = GetApplication();
        if (!app)
            return 0;

        FFontAtlas* atlas = app->GetFontAtlas().Get();
        FFont font = Font();
        const f32 scale = font.GetPointSize() / (f32)FFontAtlas::kSdfRenderSize;

        f32 targetX = localX + m_ScrollOffset;
        f32 curX    = 0.0f;
        int cpIndex = 0;

        FString display = GetDisplayText();

        for (char32_t cp : display.Codepoints())
        {
            FGlyph glyph = atlas->FindOrAddGlyph(font, cp);
            if (!glyph.IsValid())
            {
                cpIndex++;
                continue;
            }

            f32 advance = glyph.Advance * scale;

            // Snap to whichever side of this glyph is closer
            if (targetX < curX + advance * 0.5f)
                return cpIndex;

            curX += advance;
            cpIndex++;
        }

        return cpIndex; // past the last character
    }

    void FTextInput::EnsureCursorVisible()
    {
        FMargin pad = Padding();
        FVec2   sz  = GetLayoutSize();
        f32 contentWidth = FMath::Max(0.0f, sz.x - pad.left - pad.right);

        f32 cx = CursorPixelX(m_CursorPos);

        if (cx < m_ScrollOffset)
            m_ScrollOffset = cx;
        else if (cx > m_ScrollOffset + contentWidth)
            m_ScrollOffset = cx - contentWidth;

        m_ScrollOffset = FMath::Max(0.0f, m_ScrollOffset);
    }

    void FTextInput::ResetBlink()
    {
        m_CursorVisible = true;
        if (m_BlinkTimer)
            m_BlinkTimer->Reset();
    }

    // -----------------------------------------------------------------------
    // Editing
    // -----------------------------------------------------------------------

    void FTextInput::InsertAtCursor(const FString& text)
    {
        if (HasSelection())
            DeleteSelection();

        // Enforce max length
        int maxLen = MaxLength();
        if (maxLen > 0 && CpCount(m_Text) + CpCount(text) > maxLen)
            return;

        std::string s = m_Text.ToStdString();
        int byteOffset = CpToByteOffset(s.c_str(), m_CursorPos);
        s.insert(byteOffset, text.CStr(), text.ByteLength());

        m_Text      = FString(s);
        m_CursorPos += CpCount(text);

        EnsureCursorVisible();
        MarkPaintDirty();
    }

    void FTextInput::DeleteSelection()
    {
        if (!HasSelection())
            return;

        int selMin = SelectionMin();
        int selMax = SelectionMax();

        std::string s = m_Text.ToStdString();
        int byteMin = CpToByteOffset(s.c_str(), selMin);
        int byteMax = CpToByteOffset(s.c_str(), selMax);
        s.erase(byteMin, byteMax - byteMin);

        m_Text             = FString(s);
        m_CursorPos        = selMin;
        m_SelectionAnchor  = -1;

        EnsureCursorVisible();
        MarkPaintDirty();
    }

    void FTextInput::DeleteBackward()
    {
        if (HasSelection())
        {
            DeleteSelection();
            return;
        }

        if (m_CursorPos == 0)
            return;

        std::string s = m_Text.ToStdString();
        int byteEnd   = CpToByteOffset(s.c_str(), m_CursorPos);
        int byteStart = CpToByteOffset(s.c_str(), m_CursorPos - 1);
        s.erase(byteStart, byteEnd - byteStart);

        m_Text = FString(s);
        m_CursorPos--;

        EnsureCursorVisible();
        MarkPaintDirty();
    }

    void FTextInput::DeleteForward()
    {
        if (HasSelection())
        {
            DeleteSelection();
            return;
        }

        int totalCp = CpCount(m_Text);
        if (m_CursorPos >= totalCp)
            return;

        std::string s = m_Text.ToStdString();
        int byteStart = CpToByteOffset(s.c_str(), m_CursorPos);
        int byteEnd   = CpToByteOffset(s.c_str(), m_CursorPos + 1);
        s.erase(byteStart, byteEnd - byteStart);

        m_Text = FString(s);

        EnsureCursorVisible();
        MarkPaintDirty();
    }

    // -----------------------------------------------------------------------
    // Paint
    // -----------------------------------------------------------------------

    void FTextInput::Paint(FPainter& painter)
    {
        Super::Paint(painter); // Background + Border

        Ref<FApplicationInstance> app = GetApplication();
        if (!app)
            return;

        FFontAtlas*  atlas   = app->GetFontAtlas().Get();
        FFont        font    = Font();
        FFontMetrics metrics = atlas->GetScaledMetrics(font);

        FMargin pad = Padding();
        FVec2   sz  = GetLayoutSize();

        FRect contentRect = FRect::FromSize(
            pad.left,
            pad.top,
            sz.x - pad.left - pad.right,
            sz.y - pad.top  - pad.bottom
        );

        if (contentRect.GetWidth() <= 0 || contentRect.GetHeight() <= 0)
            return;

        painter.PushClip(contentRect, FRectangle());

        // Baseline: vertically center the line (ascender from top of content)
        f32 textY = contentRect.top
                  + (contentRect.GetHeight() - metrics.LineHeight) * 0.5f
                  + metrics.Ascender;

        bool focused = TestStyleState(EStyleState::Focused);

        // --- Placeholder or text ---

        if (m_Text.Empty())
        {
            if (!Placeholder().Empty())
            {
                painter.SetFont(font);
                painter.SetPen(FPen::Solid(PlaceholderColor()));
                painter.DrawText(FVec2(contentRect.left, textY), Placeholder());
            }
        }
        else
        {
            FString display = GetDisplayText();

            // Selection background
            if (HasSelection() && focused)
            {
                f32 selX0 = CursorPixelX(SelectionMin()) - m_ScrollOffset;
                f32 selX1 = CursorPixelX(SelectionMax()) - m_ScrollOffset;

                painter.SetBrush(FBrush(SelectionColor()));
                painter.SetPen(FPen());
                painter.FillRect(FRect::FromSize(
                    contentRect.left + selX0,
                    contentRect.top,
                    selX1 - selX0,
                    contentRect.GetHeight()
                ));
            }

            // Text
            painter.SetFont(font);
            painter.SetPen(FPen::Solid(TextColor()));
            painter.DrawText(FVec2(contentRect.left - m_ScrollOffset, textY), display);
        }

        // Cursor — drawn last so it's on top of selection and text
        if (focused && m_CursorVisible)
        {
            f32 cursorX = CursorPixelX(m_CursorPos) - m_ScrollOffset;
            painter.SetBrush(FBrush(CursorColor()));
            painter.SetPen(FPen());
            painter.FillRect(FRect::FromSize(
                contentRect.left + cursorX,
                contentRect.top + 2.0f,
                1.5f,
                contentRect.GetHeight() - 2.0f
            ));
        }

        painter.PopClip();
    }

    // -----------------------------------------------------------------------
    // Events
    // -----------------------------------------------------------------------

    FEventReply FTextInput::OnMouseButtonDown(FMouseEvent& event)
    {
        if (!event.IsLeftButton())
            return FEventReply::Unhandled();

        // Convert surface-space position to widget-local coordinates
        FVec2 localPos = GetGlobalTransform().Inverse().TransformPoint(event.MousePosition);

        FMargin pad  = Padding();
        f32     textX = localPos.x - pad.left;

        m_CursorPos       = HitTestCursorIndex(textX);
        m_SelectionAnchor = m_CursorPos; // will become selection if mouse moves
        m_IsDragging      = true;

        ResetBlink();
        MarkPaintDirty();

        return FEventReply::Handled().FocusSelf().CaptureMouse();
    }

    FEventReply FTextInput::OnMouseMove(FMouseEvent& event)
    {
        if (!m_IsDragging)
            return FEventReply::Unhandled();

        FVec2 localPos = GetGlobalTransform().Inverse().TransformPoint(event.MousePosition);

        FMargin pad   = Padding();
        f32     textX = localPos.x - pad.left;

        int newPos = HitTestCursorIndex(textX);
        if (newPos != m_CursorPos)
        {
            m_CursorPos = newPos;
            EnsureCursorVisible();
            MarkPaintDirty();
        }

        return FEventReply::Handled();
    }

    FEventReply FTextInput::OnMouseButtonUp(FMouseEvent& event)
    {
        if (!event.IsLeftButton())
            return FEventReply::Unhandled();

        m_IsDragging = false;

        // If cursor never moved from anchor it was a plain click — clear selection
        if (m_SelectionAnchor == m_CursorPos)
            m_SelectionAnchor = -1;

        return FEventReply::Handled().ReleaseMouse();
    }

    FEventReply FTextInput::OnKeyDown(FKeyEvent& event)
    {
        bool shift = FEnumHasFlag(event.Modifiers, EKeyModifier::Shift);
        bool ctrl  = FEnumHasFlag(event.Modifiers, EKeyModifier::Ctrl)
                  || FEnumHasFlag(event.Modifiers, EKeyModifier::Gui); // Cmd on Mac

        int totalCp = CpCount(m_Text);

        // Moves cursor to newPos. If shift is held, extends selection; otherwise
        // collapses any existing selection to the appropriate end.
        auto MoveCursor = [&](int newPos)
        {
            newPos = FMath::Clamp(newPos, 0, totalCp);

            if (shift)
            {
                if (m_SelectionAnchor == -1)
                    m_SelectionAnchor = m_CursorPos;
                m_CursorPos = newPos;
            }
            else if (HasSelection())
            {
                // Collapse: Left-ish → SelectionMin, Right-ish → SelectionMax
                m_CursorPos       = (newPos <= m_CursorPos) ? SelectionMin() : SelectionMax();
                m_SelectionAnchor = -1;
            }
            else
            {
                m_CursorPos = newPos;
            }

            EnsureCursorVisible();
            ResetBlink();
            MarkPaintDirty();
        };

        switch (event.Key)
        {
        // --- Navigation ---

        case EKeyCode::Left:
        {
            if (ctrl)
            {
                // Word backward: skip spaces then skip non-spaces
                int pos = m_CursorPos;
                std::string s = m_Text.ToStdString();
                while (pos > 0 && isspace((uint8_t)s[CpToByteOffset(s.c_str(), pos - 1)]))
                    pos--;
                while (pos > 0 && !isspace((uint8_t)s[CpToByteOffset(s.c_str(), pos - 1)]))
                    pos--;
                MoveCursor(pos);
            }
            else
            {
                MoveCursor(m_CursorPos - 1);
            }
            return FEventReply::Handled();
        }

        case EKeyCode::Right:
        {
            if (ctrl)
            {
                // Word forward: skip non-spaces then skip spaces
                int pos = m_CursorPos;
                std::string s = m_Text.ToStdString();
                while (pos < totalCp && !isspace((uint8_t)s[CpToByteOffset(s.c_str(), pos)]))
                    pos++;
                while (pos < totalCp && isspace((uint8_t)s[CpToByteOffset(s.c_str(), pos)]))
                    pos++;
                MoveCursor(pos);
            }
            else
            {
                MoveCursor(m_CursorPos + 1);
            }
            return FEventReply::Handled();
        }

        case EKeyCode::Home:
            MoveCursor(0);
            return FEventReply::Handled();

        case EKeyCode::End:
            MoveCursor(totalCp);
            return FEventReply::Handled();

        // --- Deletion ---

        case EKeyCode::Backspace:
            DeleteBackward();
            ResetBlink();
            return FEventReply::Handled();

        case EKeyCode::Delete:
            DeleteForward();
            ResetBlink();
            return FEventReply::Handled();

        // --- Select all ---

        case EKeyCode::A:
            if (ctrl)
            {
                m_SelectionAnchor = 0;
                m_CursorPos       = totalCp;
                ResetBlink();
                MarkPaintDirty();
                return FEventReply::Handled();
            }
            break;

        default:
            break;
        }

        return FEventReply::Unhandled();
    }

    FEventReply FTextInput::OnTextInput(FTextInputEvent& event)
    {
        InsertAtCursor(event.Text);
        ResetBlink();
        return FEventReply::Handled();
    }

    void FTextInput::OnFocusChanged(FFocusEvent& event)
    {
        SetStyleStateFlag(EStyleState::Focused, event.GotFocus());

        if (event.GotFocus())
        {
            if (Ref<FSurface> surface = GetParentSurface())
                surface->RequestTextInput();

            // Lazy-create the blink timer once
            if (!m_BlinkTimer)
            {
                FAssignNew(FTimer, m_BlinkTimer)
                    .Interval(0.53f)
                    .Loop(true)
                    .OnTick([this]
                    {
                        m_CursorVisible = !m_CursorVisible;
                        MarkPaintDirty();
                    });
            }

            m_CursorVisible = true;
            m_BlinkTimer->Reset();
            m_BlinkTimer->Start();
        }
        else
        {
            if (Ref<FSurface> surface = GetParentSurface())
                surface->ReleaseTextInput();

            if (m_BlinkTimer)
                m_BlinkTimer->Stop();

            m_CursorVisible   = false;
            m_SelectionAnchor = -1;
            m_IsDragging      = false;
            MarkPaintDirty();
        }
    }

} // namespace Fusion
