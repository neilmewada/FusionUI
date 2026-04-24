# Fusion Text Rendering Architecture

## Overview

This document specifies the architecture for Fusion's text rendering system. The design goals are:

- **High performance** — lazy layout, incremental invalidation, viewport culling
- **Scalability** — from a single-character label to a 100,000-line code editor
- **Simplicity** — simple widgets pay no cost for complexity they don't need
- **Multiplayer-ready** — a single plug point allows OT/CRDT collaborative editing in the future without touching any other layer

---

## Stack B — Diagram

```
┌─────────────────────────────────┐
│            Widget               │  Thin shell. Owns the layers below,
│  FTextEditor / FTextBlock / ... │  handles Fusion layout & paint calls.
├─────────────────────────────────┤
│       FTextEditController       │  Owns cursor, selection, scroll state.
│       (editable only)           │  Handles keyboard, mouse, IME input.
│                                 │  Calls document.ApplyOp() — never
│                                 │  touches the buffer directly.
├─────────────────────────────────┤
│        FTextDecorationSet       │  Visual overlays: selection highlight,
│                                 │  cursor, syntax colors, squiggles,
│                                 │  ghost text. Separate from layout so
│                                 │  they update without re-layout.
├──────────────────┬──────────────┤
│   FTextLayout    │ FTextRenderer│  Layout: converts document lines into
│                  │              │  positioned glyph runs. Lazy (visible
│                  │              │  range only) and incremental.
│                  │              │
│                  │              │  Renderer: stateless. Culls invisible
│                  │              │  lines, submits draw calls to FPainter.
├──────────────────┴──────────────┤
│          FTextDocument          │  The only component that touches the
│                                 │  buffer. Op pipeline, line index,
│                                 │  undo stack, multiplayer hook.
├─────────────────────────────────┤
│     IFTextBuffer (FLineBuffer)  │  Raw storage. Array of lines.
└─────────────────────────────────┘
```

---

## Two Stacks

The most important architectural decision: **simple and complex text do not share internal layers.** Every serious framework (Qt, WPF, browsers) makes this split. Fighting it produces something bad at both ends.

```
Stack A — Lightweight        FLabel, FTextInput
Stack B — Plain/Code         FTextEditor  (FTextBlock, FCodeBlock, FLogView = read-only modes)
```

Rich text editing (bold/italic/headings/lists as user-editable content) is explicitly **out of scope** for now. Read-only display of formatted text (bold, italic, color, size) is supported in both stacks via format annotations — no rich document model required.

---

## Primitive Widgets

| Widget | Stack | Description |
|---|---|---|
| `FLabel` | A | Read-only text display, single or multi-line, optional wrap, optional inline formatting |
| `FTextInput` | A | Single-line editable plain text field |
| `FTextEditor` | B | Multi-line editable document with virtual scroll |

### Higher-Level Widgets (configurations of the primitives)

| Widget | Built From | Notes |
|---|---|---|
| `FTextBlock` | `FLabel` | `Multiline = true` |
| `FRichTextBlock` | `FLabel` | Format annotations fed from a markup parser |
| `FCodeBlock` | `FTextEditor` | Read-only, syntax highlight decorations fed externally |
| `FLogView` | `FTextEditor` | Read-only, ring buffer, append-only |
| `FTextArea` | `FTextEditor` | No gutter or line numbers |
| `FInlineTextEdit` | `FLabel` + `FTextInput` | Swaps on click |
| `FNumericInput` | `FTextInput` | Validation and format layer on top |

---

## Stack A — Lightweight

No document model. No layout objects. No layers. Everything is inline. Simple widgets pay no cost for complexity they don't need.

---

### FLabel

Owns a string. Renders directly via `FPainter`.

**Single-line:**
```
FString
  └── FPainter (inline)
        iterate codepoints → sum advances → draw glyphs
```

**Multi-line:**
```
FString + TArray<FTextFormatRange>
  └── word-break algorithm (inline)
        respects format range boundaries for font/size changes
        produces lines of FGlyphRun
          └── FPainter — draw each line
```

#### Inline Formatting — FTextFormatRange

Format annotations are inputs to the layout step, not decorations, because font and size changes affect glyph metrics and therefore where line breaks occur.

```cpp
struct FTextFormatRange
{
    u32    StartOffset;  // byte offset into string
    u32    EndOffset;
    bool   Bold         = false;
    bool   Italic       = false;
    FColor Color        = FColor::None();  // None = inherit from widget
    float  FontSize     = 0.f;            // 0 = inherit from widget
};
```

`FLabel` accepts a `TArray<FTextFormatRange>` alongside the string. The layout step splits glyph runs at format boundaries and selects the right font variant (Bold, Italic, BoldItalic) from `FFontAtlas`. Without format ranges, there is zero overhead — single font, single color, inline rendering.

**What FLabel does NOT have:**
- No document model
- No edit controller
- No undo stack
- No multiplayer hook

---

### FTextInput

Owns a `FGapBuffer`. Cursor and selection are plain integers. Renders a single line directly.

```
FGapBuffer
  └── cursorOffset : u32
      anchorOffset : u32      = cursorOffset when no selection
      scrollX      : f32
        └── FPainter (inline)
              1. selection background quad
              2. visible glyphs (codepoints in scroll window)
              3. cursor rect
```

**Keyboard handling (inline):**
- Movement — arrows, home/end, Ctrl+arrows (word jump) → update `cursorOffset`
- Selection — Shift + any movement → extend via `anchorOffset`
- Insert — `FGapBuffer::Insert()`, advance cursor
- Delete / Backspace — `FGapBuffer::Delete()`
- Cut / Copy / Paste — clipboard ops on selected byte range
- Undo — optional single-level (store last op only)

**Mouse handling (inline):**
- Press — hit-test by summing glyph advances until X is exceeded → set `cursorOffset`
- Drag — extend selection from `anchorOffset`

**Widget properties:** `Placeholder`, `IsPassword` (bullet masking), `MaxLength`, `IsReadOnly`.

**No format annotations** — `FTextInput` is plain text only.

**What FTextInput does NOT have:**
- No document model
- No layout engine
- No decoration set
- No multiplayer hook

---

## Stack B — Plain/Code Document

Five components, each with one job. Used by `FTextEditor` in editable mode. Read-only variants (`FTextBlock`, `FCodeBlock`, `FLogView`) use the same stack with no edit controller.

---

### Component 1 — IFTextBuffer / FLineBuffer

The bottom of the stack. Raw storage, nothing else. No knowledge of cursors, layout, or rendering.

`FLineBuffer` is an array of strings — one string per line. When you type a character, a string in that array gets a character inserted. When you press Enter, one string splits into two. When you press Backspace at the start of a line, two strings merge into one.

The reason it sits behind `IFTextBuffer` is so you can swap implementations later — a ring buffer for `FLogView`, or a rope if you ever hit performance limits on extremely large files.

---

### Component 2 — FTextDocument

The gatekeeper. The only component that is allowed to touch the buffer directly. Everything above it goes through `FTextDocument`.

Three jobs:

**Op pipeline.** Every edit — whether from the user typing, pasting, or a future remote collaborator — flows through `ApplyOp(FTextOp)`. One function, one path. This gives you undo, the multiplayer hook, and change notifications all in one place for free.

**Line index.** Maintains an array of byte offsets, one per line. Lets anything above it ask "which line is byte offset 4523 on?" or "what byte offset does line 200 start at?" in O(log n) without scanning the entire document.

**Change notification.** After every op, fires `OnChanged(affectedRange)` telling exactly which byte range changed. The layout engine listens to this and marks only the affected lines dirty — not the whole document.

```
FTextDocument
  ├── IFTextBuffer*
  │     ├── FLineBuffer     array of FString, one per line   ← default
  │     └── FRingBuffer     append-only, bounded memory      ← FLogView only
  │
  ├── TArray<u32> lineOffsets
  │     byte offset of each \n, kept in sync after every op
  │     LineAtOffset / OffsetAtLine = O(log n) binary search
  │
  ├── Op pipeline
  │     ApplyOp(FTextOp) — the only mutation path, never raw buffer calls
  │
  ├── TArray<FUndoEntry>    inverse FTextOp pairs  ← editable mode only
  │
  └── IFTextDocumentBackend*    nullptr by default
                                OT / CRDT plug point for future multiplayer
```

`OnChanged(FTextRange)` fires after every op with the exact affected byte range. The layout engine uses this to invalidate only the lines that changed.

#### Why FLineBuffer and not FRopeBuffer

Monaco (VS Code) uses a plain array of line strings for a production code editor with hundreds of thousands of lines. O(1) line lookup by index is what virtual scroll needs. A rope adds significant implementation complexity for a benefit that only matters at extreme file sizes. `IFTextBuffer` is still an interface — `FRopeBuffer` can be swapped in later without touching anything else.

```cpp
enum class ETextOpType { Insert, Delete };

struct FTextOp
{
    ETextOpType Type;
    u32         ByteOffset;
    FString     Text;    // Insert: text to insert
    u32         Length;  // Delete: byte count to remove
};

class IFTextDocumentBackend
{
public:
    virtual ~IFTextDocumentBackend() = default;
    virtual void OnOpApplied(const FTextOp& op) = 0;
};

class FTextDocument
{
public:
    explicit FTextDocument(UniquePtr<IFTextBuffer> buffer);

    void ApplyOp(const FTextOp& op);
    void Undo();
    void Redo();

    u32         LineCount() const;
    u32         LineAtOffset(u32 byteOffset) const;   // O(log n)
    u32         OffsetAtLine(u32 lineIndex) const;     // O(log n)
    FStringView Read(u32 byteOffset, u32 length) const;
    u32         ByteLength() const;

    void SetBackend(IFTextDocumentBackend* backend);  // nullptr = standalone

    FEvent<FTextRange> OnChanged;
};
```

---

### Component 3 — FTextLayout

The brain of the stack. Takes the raw text from `FTextDocument` and figures out where every glyph goes on screen.

Two key behaviors:

**Lazy.** It only lays out the lines you ask for. If your document has 100,000 lines but only 40 are visible, `FTextLayout` only does work for those 40. The rest get an estimated height so the scrollbar stays accurate, but no glyph positioning happens.

**Incremental.** When `FTextDocument` fires `OnChanged(range)`, `FTextLayout` marks only the lines that overlap that range as dirty. The next build re-lays only those lines. Editing line 500 in a 100,000 line file invalidates maybe 2-3 lines, not the whole document.

The output is `FTextLine[]` — each line contains `FGlyphRun[]`, which are contiguous groups of glyphs sharing the same font and color. This is what the renderer consumes.

It also answers spatial questions for the edit controller: what pixel X is the cursor at, which byte offset is under a mouse position, which line is at a given Y coordinate.

**Inputs:**
- `FTextDocument*`
- `TArray<FTextFormatRange>` — format annotations (bold/italic/size/color)
- `float maxWidth`
- `EWrapMode`

**Outputs:**
- `TArray<FTextLine>`, each containing `TArray<FGlyphRun>`
- `float TotalHeight`

Format ranges that change font or size **split glyph run boundaries** and affect where line breaks fall. This is why they are layout inputs, not decorations. Color-only ranges also split runs but do not affect metrics.

```cpp
struct FGlyphRun
{
    FFont       Font;           // selected from format range (bold/italic/size)
    FColor      Color;          // from format range or widget default
    u32         StartOffset;    // byte offset in document
    TArray<u32> GlyphIndices;
    TArray<f32> GlyphX;         // x position of each glyph in line-local space
};

struct FTextLine
{
    f32               BaselineY;
    f32               Height;
    TArray<FGlyphRun> Runs;
};

enum class EWrapMode { None, Word, Character };

class FTextLayout
{
public:
    void SetDocument(FTextDocument* document);
    void SetFormatRanges(const TArray<FTextFormatRange>& ranges);
    void SetFont(const FFont& font);
    void SetMaxWidth(f32 maxWidth);
    void SetWrapMode(EWrapMode mode);

    void Build(u32 firstLine, u32 lastLine);   // lazy: only visible range
    void OnDocumentChanged(FTextRange range);  // incremental: dirty affected lines only

    const TArray<FTextLine>& GetLines() const;
    f32                      GetTotalHeight() const;
    f32                      GetMaxLineWidth() const;

    // Edit controller lookups
    f32 XForOffset(u32 byteOffset) const;
    u32 OffsetAtPoint(Vec2 localPos) const;
    u32 LineAtY(f32 y) const;
    u32 LineForOffset(u32 byteOffset) const;
};
```

**Lazy build:** `Build(firstLine, lastLine)` lays out only the requested range. Lines outside the range use estimated heights from `FontMetrics.LineHeight` so scrollbars remain accurate.

**Incremental invalidation:** `OnDocumentChanged(FTextRange)` marks only the lines that overlap the changed range as dirty. The next `Build()` call re-lays only those lines.

**Text shaping** (ligatures, kerning, BiDi) lives here as an implementation detail. Today Fusion sums glyph advances. HarfBuzz can be plugged in later without touching any other component.

Depends on `FFontAtlas` (existing) for glyph metrics and UV coordinates.

---

### Component 4 — FTextDecorationSet

A separate bag of visual overlays that sits alongside the layout engine. Critically — it is **not** part of layout. Changing decorations never triggers a layout rebuild.

This distinction matters because decorations change constantly — every keystroke moves the cursor, every character typed moves the selection. If that triggered re-layout, glyph positions would be rebuilt on every frame. Instead, decorations are just data the renderer reads directly.

Contains four things:
- **Highlights** — colored background ranges: selection, find/replace matches, syntax token colors, bracket matching
- **Squiggles** — underline ranges: errors, warnings, spellcheck
- **Cursors** — one or more cursor positions with colors. Local cursor today, remote cursors slot in here when multiplayer is added
- **GhostText** — text rendered after the cursor that isn't in the document: IME composition, AI suggestions

External systems write into this — the syntax highlighter writes highlight ranges, the language server writes squiggles, the edit controller writes the cursor and selection.

```cpp
struct FTextRangeColor
{
    u32    StartOffset;
    u32    EndOffset;
    FColor Color;
};

struct FTextSquiggle
{
    u32    StartOffset;
    u32    EndOffset;
    FColor Color;
    // ESquiggleStyle (wavy, straight) — add later
};

struct FCursorInfo
{
    u32    ByteOffset;
    FColor Color;
    bool   IsLocal;
    // FString UserName — for remote cursors in the future
};

class FTextDecorationSet
{
public:
    TArray<FTextRangeColor> Highlights;  // selection bg, find/replace, syntax colors
    TArray<FTextSquiggle>   Squiggles;   // errors, warnings, spellcheck
    TArray<FCursorInfo>     Cursors;     // local now; remote cursors slot in here later
    FStringView             GhostText;   // IME composition, AI suggestion
};
```

**Distinction from format annotations:** decorations are pure visual overlays — they never affect layout or line breaks. Format annotations do.

**Producers:**

| Producer | Writes |
|---|---|
| `FTextEditController` | Selection highlight, local cursor |
| App-provided syntax highlighter | `Highlights` (token color ranges) |
| App-provided diagnostics | `Squiggles` (error/warning ranges) |
| IME handler | `GhostText` |
| Future `FAwarenessSync` | Remote `FCursorInfo` entries — nothing else changes |

---

### Component 5 — FTextRenderer

The simplest component. Stateless — it owns nothing, stores nothing. Takes the layout and decorations, throws away everything outside the visible viewport, and submits draw calls to `FPainter`.

Draw order matters — highlights go behind text, squiggles go on top:
1. Highlight backgrounds (selection, find/replace — behind everything)
2. Glyph quads (the actual text)
3. Squiggle underlines (on top of text)
4. Cursor rects
5. Ghost text

The viewport culling is the entire performance story for large documents. A 100,000 line file — the renderer loops over `FTextLine[]`, skips every line whose Y position is outside the scroll window, and submits draw calls for the ~40 that are visible. The GPU never sees the other 99,960 lines.

```cpp
class FTextRenderer
{
public:
    void Draw(
        FPainter&                 painter,
        const FTextLayout&        layout,
        const FTextDecorationSet& decorations,
        Rect                      viewport,
        Vec2                      origin
    );
};
```

**Draw order per visible line:**
1. Highlight background quads (selection, find/replace)
2. Glyph quads — existing `FPainter` SDF path, per `FGlyphRun`
3. Squiggle underlines
4. Cursor rects
5. Ghost text glyphs

Lines whose Y range falls entirely outside `viewport` are skipped. This is the primary performance win for large documents — the layout engine may have 100,000 lines but the renderer submits draw calls for only the ~40 visible ones.

Replaces the current character loop inside `FPainter::DrawText()`.

---

### Component 6 — FTextEditController

Present only in editable mode (`FTextEditor`). Read-only variants simply don't instantiate one.

Owns everything about the user's editing state — where the cursor is, what's selected, how far the document is scrolled. Three jobs:

**Input translation.** Receives raw platform events — key presses, mouse clicks, scroll events, IME composition — and translates them into either cursor movements or document operations. Arrow key becomes a cursor offset change. A character key becomes `ApplyOp(Insert)`. Backspace becomes `ApplyOp(Delete)`.

**Cursor and selection.** Maintains cursor offset, selection anchor offset, and scroll position. Uses `FTextLayout` to answer spatial questions — where on screen is this offset, what offset is under this mouse click. The layout engine does the spatial math, the controller does the state management.

**Decoration production.** Each frame, `BuildDecorationSet()` produces the `FTextDecorationSet` — the selection highlight range and cursor position — which flows into `FTextRenderer`.

All actual document mutations go through `FTextDocument.ApplyOp()`. The edit controller never touches the buffer directly. This means undo, the change notification, and the future multiplayer hook all activate automatically on every edit without the controller doing anything special.

```cpp
class FTextEditController
{
public:
    explicit FTextEditController(FTextDocument* document, FTextLayout* layout);

    void HandleKeyEvent(const FKeyEvent& event);
    void HandleMousePress(Vec2 localPos, int clickCount);
    void HandleMouseDrag(Vec2 localPos);
    void HandleTextInput(FStringView text);   // direct char / IME composition
    void HandleScroll(Vec2 delta);

    FTextDecorationSet BuildDecorationSet() const;

    u32  CursorOffset() const;
    u32  AnchorOffset() const;
    Vec2 ScrollOffset() const;
    bool HasSelection() const;
    void ClearSelection();
};
```

---

### The Widget

The thinnest layer. `FTextEditor`, `FTextBlock`, `FCodeBlock` are all just shells that:

1. Own the components above
2. Wire them together — subscribe `FTextLayout` to `FTextDocument.OnChanged`, etc.
3. Handle Fusion's layout system (`MeasureContent`, `ArrangeContent`)
4. Call `FTextRenderer.Draw()` from `Paint()`
5. Forward input events to `FTextEditController`
6. Expose a clean public API (`SetText()`, `OnTextChanged`, etc.)

The widget contains almost no logic itself. All the real work happens in the components.

---

### Read-Only Variants of Stack B

| Widget | Buffer | Edit Controller | Undo | Format Annotations |
|---|---|---|---|---|
| `FTextBlock` | `FLineBuffer` | — | — | yes |
| `FCodeBlock` | `FLineBuffer` | — | — | yes (syntax highlighter → `FTextDecorationSet`) |
| `FLogView` | `FRingBuffer` | — | — | no |
| `FTextEditor` | `FLineBuffer` | yes | yes | yes |

---

## Data Flow

### Write path (keystroke → screen)

```
Keystroke
  → FTextEditController.HandleKeyEvent()
      → FTextDocument.ApplyOp()
            → IFTextBuffer mutated
            → lineOffsets updated
            → IFTextDocumentBackend.OnOpApplied()   ← no-op today; OT/CRDT in future
            → OnChanged(affectedRange) fired
                  → FTextLayout.OnDocumentChanged(affectedRange)
                       affected lines marked dirty

Per frame (Paint):
  → FTextLayout.Build(visibleLineRange)         lazy, incremental
  → FTextEditController.BuildDecorationSet()
  → FTextRenderer.Draw(layout, decorations, viewport, origin)
        → FPainter → FUIDrawList → GPU
```

### Read path (click to place cursor)

```
Mouse click at (x, y)
  → FTextEditController.HandleMousePress(localPos)
      → FTextLayout.OffsetAtPoint(localPos)  → byteOffset
      → CursorOffset = byteOffset
      → decoration set updated next frame
```

### Format annotation path (syntax highlighting, markup)

```
App parses document content
  → produces TArray<FTextFormatRange>
  → FTextLayout.SetFormatRanges(ranges)
  → FTextLayout.OnDocumentChanged(fullRange)   forces re-lay of affected lines
  → next Build() picks up new font/color splits in FGlyphRun
```

---

## Future Multiplayer

Two additions below `FTextDocument`. Nothing else changes.

```
IFTextDocumentBackend        — already in FTextDocument, nullptr today
└── FCollabDocument          — OT or CRDT state, op log, merge logic
     └── IFDocumentTransport — network transport (app-provided, pluggable)

FAwarenessSync               — ephemeral remote cursor/selection channel
                               writes FCursorInfo into FTextDecorationSet.Cursors
                               (the slot already exists)
```

**OT vs CRDT:** If multiplayer assumes a central server (typical for apps), OT is simpler to implement correctly. If peer-to-peer or offline-first is needed, CRDT is the right choice. Either implementation satisfies `IFTextDocumentBackend` — the rest of Fusion is unaffected.

**Undo note:** Local undo in a collaborative document requires selective undo (undo only your own ops, rebasing around others). The current simple undo stack will need replacement at that point. This is a known future cost.

---

## Existing Code Affected

| Existing | Change |
|---|---|
| `FPainter::DrawText()` | Character loop extracted into `FTextRenderer`. `FPainter` retains the SDF quad submission path. |
| `FLabel` | Rewritten as a lightweight direct-render widget using `FTextFormatRange` for inline formatting. No document model. |
| `FTextInput` | `FGapBuffer` replaces internal string. Cursor/selection remain as plain integers, inline in the widget. |
| `FFontAtlas` | No change. Used by `FTextLayout` as a dependency. Bold/italic font variants must be loaded. |
| `FUIDrawList` / shaders | No change. |
