# Slint UI Generator - Agent Memory

## Key Patterns

- [Slint Syntax Notes](slint-syntax.md) - Critical syntax rules and pitfalls discovered during code generation
- [Design Token Patterns](design-tokens.md) - How to structure global tokens for Slint projects

## Validation Notes
- slint-compiler 1.15.1 is available at /usr/bin/slint-compiler
- `padding-top` on Text elements produces a warning (padding only affects layout containers); use parent spacing/gap instead
- Unicode escapes (\uXXXX) are NOT parsed in Slint string literals; always use actual UTF-8 characters
- `margin-top` does not exist in Slint; use parent container padding or spacing to create gaps

## Layout Centering Pitfalls (Critical)

### 1. Horizontal Centering in VerticalLayout
`VerticalLayout { alignment: center; }` does **NOT** horizontally center fixed-width children. It only aligns children along the **main axis** (vertical). Fixed-width children default to left alignment on the cross axis.

**Correct approach**: Explicitly compute `x` for any fixed-width child:
```slint
Rectangle {
    x: (parent.width - self.width) / 2;
    width: 210px;
    height: 64px;
    // ...
}
```

### 2. Vertical Centering of Text in HorizontalLayout
`HorizontalLayout { width: 100%; height: 100%; alignment: center; }` will center the Text element's bounding box, but Slint `Text` defaults to `vertical-alignment: top` (rendering from the baseline/box top). The text will still appear visually offset upward.

**Correct approach**: Always add `vertical-alignment: center;` to Text elements that must look vertically centered inside a layout:
```slint
Text {
    text: "취소";
    font-size: 18px;
    vertical-alignment: center;
}
```

### 3. Vertical Centering of Image in HorizontalLayout (Critical)
`Image` does **NOT** support `vertical-alignment: center;`. Writing it on an `Image` inside a `HorizontalLayout` has no effect and the image will stick to the top, appearing misaligned.

**Correct approach**: For `Image` elements that need to look vertically centered inside a `HorizontalLayout`, explicitly compute `y`:
```slint
Image {
    width: 24px;
    height: 24px;
    y: (parent.height - self.height) / 2;
    source: @image-url("../assets/images/example.png");
}
```

### 4. Image Asset Paths Must Be Relative to the `.slint` File Location
All generated `.slint` files are written to the `ui/` directory. The `assets/images/` folder is at the repository root, which is **one level above** `ui/`. Writing `@image-url("assets/images/...")` inside `ui/*.slint` causes the image to fail to load at runtime/preview because the resolved path becomes `ui/assets/images/...`.

**Correct approach**: Always use `@image-url("../assets/images/<filename>.png")` in generated `.slint` files.

### 4. Do Not Invent or Alter Colors
Use the exact hex color values from the UI Layout Analysis Report. Do **not** change alpha channels, round colors, or "eyeball" replacements. If the report says `#10fa2f4d`, write exactly `#10fa2f4d` — not `#10fa2fbd` or any other variation.

## Icon Export Protocol (Critical)

For **EVERY** node in the layout report where `Name = Icon` or `Type = Icon`:
1. **Identify the Icon frame node ID** (the parent frame, NOT its inner Vector/path children).
2. Use **pencil MCP `export_nodes`** with `scale: 1` to export that exact node ID to PNG.
3. Save to `assets/images/<node-id-or-descriptive-name>.png`.
4. In `.slint`, use:
   ```slint
   Image {
       x: ...; y: ...; width: ...; height: ...;
       source: @image-url("../assets/images/<filename>.png");
   }
   ```
5. **NEVER use emoji placeholders** (`🎤`, `🏠`, etc.), solid `Rectangle` blocks, or handwritten `Path` elements to replace an Icon.
6. If a generated file contains any placeholder for an Icon, **it is a FAILED delivery** and must be regenerated with actual image exports.

### Vector Graphics Icon Export (NEW)

**ANY** frame containing multiple vector shapes (path, ellipse) that together form a recognizable icon **MUST** be exported as PNG:

| Scenario | Action | Example |
|----------|--------|---------|
| Frame with 3+ vector children | Export as PNG | Error face (4 ellipses + path) |
| Complex decorative shape | Export as PNG | Corner brackets, checkmark badges |
| Simple geometric shape | Can use Rectangle | Solid color button backgrounds |

**Rule of thumb**: If the icon requires 3+ Rectangle/Path elements to recreate in Slint, **export the original as PNG instead**.

## Text Wrapping Rule (Critical)

Only add `wrap: word-wrap;` to a `Text` element when **both** of the following are true:
- The Text has an explicit `height` property (not auto-sized).
- The explicit `height` is **strictly greater than 1.8 × font-size**.

**Why**: Slint's `wrap: word-wrap` can cause short single-line text (like button labels or pill text) to unexpectedly break into two lines or render with incorrect vertical centering when the bounding box is only slightly taller than one line. It should only be enabled for genuinely multi-line text blocks.

**Examples**:
- `font-size: 30px`, `height: 35.995px` → 35.995 < (1.8 × 30 = 54) → **do NOT add wrap**
- `font-size: 18px`, `height: 28px` → 28 < (1.8 × 18 = 32.4) → **do NOT add wrap**
- `font-size: 18px`, `height: 60px` → 60 > (1.8 × 18 = 32.4) → **add `wrap: word-wrap;`**

## Root Node Canvas Offset (Critical)

Pencil design files often place the screen/frame at an offset position on the canvas (e.g., `r93l7 @ (36.39, 30.22)`). This offset represents where the designer placed the artboard, NOT padding that should exist in the actual UI.

**The Problem**: If you copy this offset into the Slint code:
```slint
// WRONG - this pushes the entire UI away from the window edge
Rectangle {
    x: 36.39px;  // ← Canvas offset copied from Pencil
    y: 30.22px;
    // ...
}
```
The UI will appear off-center when the actual window size differs from the design viewport.

**Correct approach**: Ignore the root node's canvas offset. Start the UI from (0, 0) and use layouts for centering:
```slint
// CORRECT - fills the window, uses layout for alignment
export component PasswordInput inherits Window {
    preferred-width: 448px;
    preferred-height: 900px;

    Rectangle {
        width: 100%;
        height: 100%;
        // No x/y offset - fills the window

        VerticalLayout {
            alignment: center;
            // Content starts from natural position
        }
    }
}
```

**Rule**: When the layout report shows a root Container with `x > 0` or `y > 0` relative to the canvas, **DO NOT** copy these values to the Slint code. The root component should fill the window starting from (0, 0).
