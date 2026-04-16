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

    char32_t FTextInput::CpAt(const char* str, int cpIndex)
    {
        const char* ptr = str + CpToByteOffset(str, cpIndex);
        uint8_t b = (uint8_t)*ptr;
        if (b < 0x80) return (char32_t)b;
        if (b < 0xE0) return ((char32_t)(b & 0x1F) << 6)  | ((uint8_t)ptr[1] & 0x3F);
        if (b < 0xF0) return ((char32_t)(b & 0x0F) << 12) | (((uint8_t)ptr[1] & 0x3F) << 6)  | ((uint8_t)ptr[2] & 0x3F);
        return           ((char32_t)(b & 0x07) << 18) | (((uint8_t)ptr[1] & 0x3F) << 12) | (((uint8_t)ptr[2] & 0x3F) << 6) | ((uint8_t)ptr[3] & 0x3F);
    }

    bool FTextInput::IsWordChar(char32_t cp)
    {
        if (cp < 128)
            return isalnum((int)cp) || cp == '_';
        return true; // treat non-ASCII codepoints as word characters
    }

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    FTextInput::FTextInput()
    {
        m_ClipContent = true;
        SetWidgetFlag(EWidgetFlags::Focusable, true);
    }

    // -----------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------

    FVec2 FTextInput::MeasureContent(FVec2 availableSize)
    {
        ZoneScoped;
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
        ZoneScoped;
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
        ZoneScoped;
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
        ZoneScoped;
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
        ZoneScoped;
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
        ZoneScoped;
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
        ZoneScoped;
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
        m_OnTextChanged.Broadcast(m_Text);
    }

    void FTextInput::DeleteRange(int cpFrom, int cpTo)
    {
        ZoneScoped;
        if (cpFrom >= cpTo)
            return;

        std::string s = m_Text.ToStdString();
        int byteFrom = CpToByteOffset(s.c_str(), cpFrom);
        int byteTo   = CpToByteOffset(s.c_str(), cpTo);
        s.erase(byteFrom, byteTo - byteFrom);

        m_Text             = FString(s);
        m_CursorPos        = cpFrom;
        m_SelectionAnchor  = -1;

        EnsureCursorVisible();
        MarkPaintDirty();
        m_OnTextChanged.Broadcast(m_Text);
    }

    void FTextInput::DeleteSelection()
    {
        ZoneScoped;
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
        m_OnTextChanged.Broadcast(m_Text);
    }

    void FTextInput::DeleteBackward()
    {
        ZoneScoped;
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
        m_OnTextChanged.Broadcast(m_Text);
    }

    void FTextInput::DeleteForward()
    {
        ZoneScoped;
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
        m_OnTextChanged.Broadcast(m_Text);
    }

    // -----------------------------------------------------------------------
    // Paint
    // -----------------------------------------------------------------------

    void FTextInput::Paint(FPainter& painter)
    {
        ZoneScoped;
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

        bool editing = TestStyleState(EStyleState::Editing);

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
            if (HasSelection() && editing)
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
        if (editing && m_CursorVisible)
        {
            f32 cursorX      = CursorPixelX(m_CursorPos) - m_ScrollOffset;
            f32 cursorHeight = metrics.Ascender - metrics.Descender;
            f32 cursorY      = textY - metrics.Ascender;

            painter.SetBrush(FBrush(CursorColor()));
            painter.SetPen(FPen());
            painter.FillRect(FRect::FromSize(
                contentRect.left + cursorX,
                cursorY,
                1.5f,
                cursorHeight
            ));
        }

        painter.PopClip();
    }

    // -----------------------------------------------------------------------
    // Events
    // -----------------------------------------------------------------------

    FEventReply FTextInput::OnMouseButtonDown(FMouseEvent& event)
    {
        ZoneScoped;
        if (!event.IsLeftButton())
            return FEventReply::Unhandled();

        FVec2 localPos = GetGlobalTransform().Inverse().TransformPoint(event.MousePosition);
        FMargin pad    = Padding();
        f32 textX      = localPos.x - pad.left;

        if (event.ClickCount >= 3)
        {
            // Triple-click: select all
            m_SelectionAnchor = 0;
            m_CursorPos       = CpCount(m_Text);
            m_IsDragging      = false;

            EnterEditing();
            ResetBlink();
            MarkPaintDirty();
            return FEventReply::Handled().FocusSelf();
        }

        if (event.ClickCount == 2)
        {
            // Double-click: select the word (or whitespace run) under the cursor
            int totalCp = CpCount(m_Text);
            int cpIndex = FMath::Clamp(HitTestCursorIndex(textX), 0, totalCp);

            if (totalCp > 0)
            {
                // Use the character to the right of the cursor (or last char if at end)
                int charIdx = (cpIndex < totalCp) ? cpIndex : totalCp - 1;
                const char* str = m_Text.CStr();
                char32_t refCp = CpAt(str, charIdx);

                // Characters belong to the same class if they're both word chars,
                // both spaces, or both "other" (punctuation etc.)
                auto CharClass = [](char32_t cp) -> int {
                    if (cp >= 128)        return 0; // non-ASCII = word
                    if (isalnum((int)cp) || cp == '_') return 0; // word
                    if (isspace((int)cp)) return 1; // whitespace
                    return 2;                        // punctuation / other
                };

                int refClass = CharClass(refCp);

                int wordStart = cpIndex;
                while (wordStart > 0 && CharClass(CpAt(str, wordStart - 1)) == refClass)
                    wordStart--;

                int wordEnd = (cpIndex < totalCp) ? cpIndex : totalCp;
                while (wordEnd < totalCp && CharClass(CpAt(str, wordEnd)) == refClass)
                    wordEnd++;

                m_SelectionAnchor = wordStart;
                m_CursorPos       = wordEnd;
            }

            m_IsDragging = false;

            EnterEditing();
            ResetBlink();
            MarkPaintDirty();
            return FEventReply::Handled().FocusSelf();
        }

        // Single click: position cursor and begin potential drag-selection
        m_CursorPos       = HitTestCursorIndex(textX);
        m_SelectionAnchor = m_CursorPos;
        m_IsDragging      = true;

        EnterEditing();
        ResetBlink();
        MarkPaintDirty();
        return FEventReply::Handled().FocusSelf().CaptureMouse();
    }

    FEventReply FTextInput::OnMouseMove(FMouseEvent& event)
    {
        ZoneScoped;
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
        ZoneScoped;
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
        ZoneScoped;
        bool shift   = FEnumHasFlag(event.Modifiers, EKeyModifier::Shift);
        bool ctrl    = FEnumHasFlag(event.Modifiers, EKeyModifier::Ctrl);
        bool alt     = FEnumHasFlag(event.Modifiers, EKeyModifier::Alt);
#if FUSION_PLATFORM_MAC
        bool cmd     = FEnumHasFlag(event.Modifiers, EKeyModifier::Gui);
        bool wordMod = alt;   // Option+Left/Right → word jump
        bool lineMod = cmd;   // Cmd+Left/Right    → line start/end
        bool selAll  = cmd;   // Cmd+A             → select all
#else
        bool wordMod = ctrl;  // Ctrl+Left/Right   → word jump
        bool lineMod = false; // Home/End keys handle line start/end
        bool selAll  = ctrl;  // Ctrl+A            → select all
#endif
        bool editing = TestStyleState(EStyleState::Editing);

        // --- Focus-mode transitions (handled regardless of editing state) ---

        switch (event.Key)
        {
        case EKeyCode::Tab:
            // Exit editing first (if active), then let the focus system handle Tab
            if (editing)
            {
                ExitEditing();
                return FEventReply::Unhandled();
            }
            return shift ? FEventReply::Handled().FocusPrev() : FEventReply::Handled().FocusNext();

        case EKeyCode::Escape:
            if (editing)
            {
                m_Text      = m_TextBeforeEdit;
                m_CursorPos = FMath::Min(m_CursorPos, CpCount(m_Text));
                ExitEditing();
                m_OnTextCanceled.Broadcast(m_Text);
                return FEventReply::Handled();
            }
            return FEventReply::Unhandled();

        case EKeyCode::Return:
            if (editing)
            {
                ExitEditing();
                m_OnTextSubmitted.Broadcast(m_Text);
                return FEventReply::Handled();
            }
            EnterEditing();
            return FEventReply::Handled();

        default:
            break;
        }

        // --- All keys below require editing mode ---

        if (!editing)
            return FEventReply::Unhandled();

        int totalCp = CpCount(m_Text);

        // Moves cursor to newPos. If shift is held, extends/reduces the selection.
        // Does NOT handle selection collapse — callers do that before calling this.
        auto MoveCursor = [&](int newPos)
        {
            newPos = FMath::Clamp(newPos, 0, totalCp);

            if (shift)
            {
                if (m_SelectionAnchor == -1)
                    m_SelectionAnchor = m_CursorPos;
                m_CursorPos = newPos;
            }
            else
            {
                m_CursorPos       = newPos;
                m_SelectionAnchor = -1;
            }

            EnsureCursorVisible();
            ResetBlink();
            MarkPaintDirty();
        };

        // Collapses an active selection to one of its ends without moving further.
        // Must be called before MoveCursor when !shift and a selection exists.
        auto CollapseLeft  = [&]() { m_CursorPos = SelectionMin(); m_SelectionAnchor = -1; EnsureCursorVisible(); ResetBlink(); MarkPaintDirty(); };
        auto CollapseRight = [&]() { m_CursorPos = SelectionMax(); m_SelectionAnchor = -1; EnsureCursorVisible(); ResetBlink(); MarkPaintDirty(); };

        switch (event.Key)
        {
        // --- Navigation ---

        case EKeyCode::Left:
        {
            if (!shift && HasSelection())
            {
                // Collapse to left end — no further movement (standard behaviour)
                CollapseLeft();
            }
            else if (lineMod)
            {
                MoveCursor(0);
            }
            else if (wordMod)
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
            if (!shift && HasSelection())
            {
                // Collapse to right end — no further movement (standard behaviour)
                CollapseRight();
            }
            else if (lineMod)
            {
                MoveCursor(totalCp);
            }
            else if (wordMod)
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
        {
            if (HasSelection())
            {
                DeleteSelection();
            }
            else if (lineMod)
            {
                // Delete to line start
                DeleteRange(0, m_CursorPos);
            }
            else if (wordMod)
            {
                // Delete word backward
                int pos = m_CursorPos;
                std::string s = m_Text.ToStdString();
                while (pos > 0 && isspace((uint8_t)s[CpToByteOffset(s.c_str(), pos - 1)]))
                    pos--;
                while (pos > 0 && !isspace((uint8_t)s[CpToByteOffset(s.c_str(), pos - 1)]))
                    pos--;
                DeleteRange(pos, m_CursorPos);
            }
            else
            {
                DeleteBackward();
            }
            ResetBlink();
            return FEventReply::Handled();
        }

        case EKeyCode::Delete:
        {
            if (HasSelection())
            {
                DeleteSelection();
            }
            else if (lineMod)
            {
                // Delete to line end
                DeleteRange(m_CursorPos, totalCp);
            }
            else if (wordMod)
            {
                // Delete word forward
                int pos = m_CursorPos;
                std::string s = m_Text.ToStdString();
                while (pos < totalCp && !isspace((uint8_t)s[CpToByteOffset(s.c_str(), pos)]))
                    pos++;
                while (pos < totalCp && isspace((uint8_t)s[CpToByteOffset(s.c_str(), pos)]))
                    pos++;
                DeleteRange(m_CursorPos, pos);
            }
            else
            {
                DeleteForward();
            }
            ResetBlink();
            return FEventReply::Handled();
        }

        // --- Select all ---

        case EKeyCode::A:
            if (selAll)
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
        ZoneScoped;
        if (!TestStyleState(EStyleState::Editing))
            return FEventReply::Unhandled();

        InsertAtCursor(event.Text);
        ResetBlink();
        return FEventReply::Handled();
    }

    void FTextInput::EnterEditing()
    {
        ZoneScoped;
        if (TestStyleState(EStyleState::Editing))
            return;

        m_TextBeforeEdit = m_Text;
        SetStyleStateFlag(EStyleState::Editing, true);

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

        MarkPaintDirty();
    }

    void FTextInput::ExitEditing()
    {
        ZoneScoped;
        if (!TestStyleState(EStyleState::Editing))
            return;

        SetStyleStateFlag(EStyleState::Editing, false);

        if (Ref<FSurface> surface = GetParentSurface())
            surface->ReleaseTextInput();

        if (m_BlinkTimer)
            m_BlinkTimer->Stop();

        m_CursorVisible   = false;
        m_SelectionAnchor = -1;
        m_IsDragging      = false;

        MarkPaintDirty();
    }

    void FTextInput::OnFocusChanged(FFocusEvent& event)
    {
        ZoneScoped;
        SetStyleStateFlag(EStyleState::Focused, event.GotFocus());

        if (!event.GotFocus())
        {
            bool wasEditing = TestStyleState(EStyleState::Editing);
            bool textChanged = (m_Text != m_TextBeforeEdit);
            ExitEditing();
            if (wasEditing && textChanged)
                m_OnTextSubmitted.Broadcast(m_Text);
        }
        else
        {
            MarkPaintDirty();
        }
    }

} // namespace Fusion
