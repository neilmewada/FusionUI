#pragma once

namespace Fusion
{
    class FWidget;

    enum class EEventType
    {
        None = 0,

        MouseEnter,
        MouseLeave,
        MouseMove,
        MouseWheel,

        MouseButtonDown,
        MouseButtonUp,

        KeyDown,
        KeyUp,
        TextInput,

        FocusChanged,
    };
    FUSION_ENUM_CLASS(EEventType);

    // ---------------------------------------------------------------------------

    struct FEventReply
    {
    public:

        enum class MouseCaptureOp { None, Capture, Release };
        enum class InputLockOp    { None, Lock, Unlock };
        enum class FocusOp        { None, Self, Next, Prev };

        static FEventReply Handled()   { FEventReply r; r.m_Handled = true; return r; }
        static FEventReply Unhandled() { return {}; }

        // Soft mouse capture: mouse events keep routing here even when cursor leaves bounds.
        // Does NOT affect keyboard. Does NOT hide cursor.
        FEventReply& CaptureMouse(FCursor cursor = FCursor::Inherit())
        {
	        m_MouseCaptureOp = MouseCaptureOp::Capture; 
        	m_CursorOverride = MoveTemp(cursor); 
        	return *this;
        }

        FEventReply& ReleaseMouse() { m_MouseCaptureOp = MouseCaptureOp::Release; return *this; }

        // Full input lock: ALL mouse + keyboard route here. Hides and locks cursor.
        // Use for 3D viewport / game input capture.
        FEventReply& LockInput()   { m_InputLockOp = InputLockOp::Lock;   return *this; }
        FEventReply& UnlockInput() { m_InputLockOp = InputLockOp::Unlock; return *this; }

        // Keyboard focus
        FEventReply& FocusSelf() { m_FocusOp = FocusOp::Self; return *this; }
        FEventReply& FocusNext() { m_FocusOp = FocusOp::Next; return *this; }
        FEventReply& FocusPrev() { m_FocusOp = FocusOp::Prev; return *this; }

        bool           IsHandled()         const { return m_Handled; }
        FocusOp        GetFocusOp()        const { return m_FocusOp; }
        MouseCaptureOp GetMouseCaptureOp() const { return m_MouseCaptureOp; }
        FCursor        GetCursorOverride() const { return m_CursorOverride; }
        InputLockOp    GetInputLockOp()    const { return m_InputLockOp; }

    private:
        bool           m_Handled        = false;
        FocusOp        m_FocusOp        = FocusOp::None;
        MouseCaptureOp m_MouseCaptureOp = MouseCaptureOp::None;
        FCursor        m_CursorOverride = FCursor::Inherit();
        InputLockOp    m_InputLockOp    = InputLockOp::None;
    };

    // ---------------------------------------------------------------------------

    struct FUSIONWIDGETS_API FEvent
    {
    public:

        virtual ~FEvent() = default;

        EEventType   Type   = EEventType::None;
        Ref<FWidget> Sender = nullptr; // current handler — updated as event bubbles
        Ref<FWidget> Target = nullptr; // original source - only used for specific events

        bool IsMouseEvent() const
        {
            switch (Type)
            {
            case EEventType::MouseButtonDown:
            case EEventType::MouseButtonUp:
            case EEventType::MouseEnter:
            case EEventType::MouseLeave:
            case EEventType::MouseMove:
            case EEventType::MouseWheel:
                return true;
            default:
                return false;
            }
        }

        bool IsKeyEvent() const
        {
            switch (Type)
            {
            case EEventType::KeyDown:
            case EEventType::KeyUp:
            case EEventType::TextInput:
                return true;
            default:
                return false;
            }
        }

        bool IsFocusEvent() const { return Type == EEventType::FocusChanged; }
    };

    // ---------------------------------------------------------------------------

    struct FUSIONWIDGETS_API FMouseEvent : FEvent
    {
    public:

        FVec2            MousePosition;
        FVec2            PrevMousePosition;
        FVec2            WheelDelta;
        EMouseButtonMask Buttons      = EMouseButtonMask::None;
        EKeyModifier     KeyModifiers = EKeyModifier::None;
        bool             bIsInside  = false;
        int              ClickCount = 1;   // 1 = single, 2 = double, 3 = triple, etc.

        bool IsLeftButton()   const { return FEnumHasFlag(Buttons, EMouseButtonMask::Left); }
        bool IsRightButton()  const { return FEnumHasFlag(Buttons, EMouseButtonMask::Right); }
        bool IsMiddleButton() const { return FEnumHasFlag(Buttons, EMouseButtonMask::Middle); }

        bool IsMultiSelectionModifier() const
        {
#if PLATFORM_MAC
            return FEnumHasFlag(KeyModifiers, EKeyModifier::Gui | EKeyModifier::Shift);
#else
            return FEnumHasFlag(KeyModifiers, EKeyModifier::Ctrl | EKeyModifier::Shift);
#endif
        }

        bool IsCtrlMultiSelectionModifier() const
        {
#if PLATFORM_MAC
            return FEnumHasFlag(KeyModifiers, EKeyModifier::Gui);
#else
            return FEnumHasFlag(KeyModifiers, EKeyModifier::Ctrl);
#endif
        }

        bool IsShiftMultiSelectionModifier() const
        {
            return FEnumHasFlag(KeyModifiers, EKeyModifier::Shift);
        }
    };

    // ---------------------------------------------------------------------------

    struct FUSIONWIDGETS_API FKeyEvent : FEvent
    {
    public:

        EKeyCode     Key       = EKeyCode::Unknown;
        EKeyModifier Modifiers = EKeyModifier::None;
    };

    // ---------------------------------------------------------------------------

    struct FUSIONWIDGETS_API FTextInputEvent : FEvent
    {
    public:

        // UTF-8 encoded text produced by this input event.
        // May be multiple codepoints (e.g. IME commit can produce a full word at once).
        FString Text;
    };

    // ---------------------------------------------------------------------------

    struct FUSIONWIDGETS_API FFocusEvent : FEvent
    {
    public:

        bool         bGotFocus     = false;
        bool         bFromKeyboard = false;
        Ref<FWidget> FocusedWidget = nullptr;

        bool GotFocus()  const { return bGotFocus; }
        bool LostFocus() const { return !bGotFocus; }
    };

} // namespace Fusion
