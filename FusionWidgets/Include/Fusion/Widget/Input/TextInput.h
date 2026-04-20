#pragma once

namespace Fusion
{
    FUSION_SIGNAL_TYPE(FTextSignal, const FString& text);

    class FUSIONWIDGETS_API FTextInput : public FDecoratedBox
    {
        FUSION_WIDGET(FTextInput, FDecoratedBox)
    protected:

        FTextInput();

    public:

        // - Layout -

        FVec2 MeasureContent(FVec2 availableSize) override;
        void  ArrangeContent(FVec2 finalSize) override;

        // - Cursor -

        FCursor GetActiveCursorAt(FVec2 localPos) override;

    protected:

        // - Paint -

        void Paint(FPainter& painter) override;

        // - Events -

        FEventReply OnMouseButtonDown(FMouseEvent& event) override;
        FEventReply OnMouseMove(FMouseEvent& event) override;
        FEventReply OnMouseButtonUp(FMouseEvent& event) override;
        FEventReply OnKeyDown(FKeyEvent& event) override;
        FEventReply OnTextInput(FTextInputEvent& event) override;
        void        OnFocusChanged(FFocusEvent& event) override;
        void        OnEnabled() override;
        void        OnDisabled() override;

    private:

        // - UTF-8 / codepoint helpers -

        static const char* Utf8Advance(const char* ptr);
        static int         CpToByteOffset(const char* str, int cpIndex);
        static int         CpCount(const FString& str);
        static char32_t    CpAt(const char* str, int cpIndex);
        static bool        IsWordChar(char32_t cp);

        // - Text measurement -

        FString GetDisplayText() const;
        f32     MeasureTextWidth(const char* start, const char* end) const;
        f32     CursorPixelX(int cpIndex) const;
        int     HitTestCursorIndex(f32 localX) const;

        // - Editing -

        void InsertAtCursor(const FString& text);
        void DeleteRange(int cpFrom, int cpTo); // erases [cpFrom, cpTo), moves cursor to cpFrom
        void DeleteSelection();
        void DeleteBackward();
        void DeleteForward();

        // - Selection -

        bool HasSelection() const { return m_SelectionAnchor != -1 && m_SelectionAnchor != m_CursorPos; }
        int  SelectionMin() const { return FMath::Min(m_CursorPos, m_SelectionAnchor); }
        int  SelectionMax() const { return FMath::Max(m_CursorPos, m_SelectionAnchor); }
        void ClearSelection()     { m_SelectionAnchor = -1; }

        void EnterEditing();
        void ExitEditing();

        void EnsureCursorVisible();
        void ResetBlink();

        // - State -

        int     m_CursorPos       = 0;    // codepoint index in m_Text
        int     m_SelectionAnchor = -1;   // codepoint index; -1 = no selection
        f32     m_ScrollOffset    = 0.0f;
        bool    m_CursorVisible   = false;
        bool    m_IsDragging      = false;
        FString m_TextBeforeEdit;         // snapshot taken when editing starts; restored on cancel

        Ref<FTimer> m_BlinkTimer;

    public:

        // - Properties -

        FUSION_PROPERTY(FString, Text);
        FUSION_PROPERTY(FString, Placeholder);
        FUSION_PROPERTY(bool,    IsPassword);
        FUSION_PROPERTY(int,     MaxLength);

        // - Signals -

        FUSION_SIGNAL(FTextSignal, OnTextChanged);   // fired on every edit
        FUSION_SIGNAL(FTextSignal, OnTextSubmitted); // fired on Enter / Tab-out / click-away
        FUSION_SIGNAL(FTextSignal, OnTextCanceled);  // fired on Escape; text has been reverted

        FUSION_STYLE_PROPERTIES(
            (FFont,  Font,             LayoutAndPaint),
            (FColor, TextColor,        Paint),
            (FColor, PlaceholderColor, Paint),
            (FColor, SelectionColor,   Paint),
            (FColor, CursorColor,      Paint)
        );

    };

} // namespace Fusion
