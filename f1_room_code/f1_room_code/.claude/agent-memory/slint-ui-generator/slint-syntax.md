---
name: Slint Syntax Notes
description: Critical Slint syntax rules, pitfalls, and patterns discovered during code generation
type: reference
---

# Slint Syntax Notes

## Gradient Syntax
- Use `@linear-gradient(angle, color1 percentage, color2 percentage)` for gradient backgrounds.
- The `background` property on Rectangle accepts a `<brush>` type, which supports both solid colors and gradients.
- Gradient properties must be declared as `property <brush>` in globals, not `property <color>`.
- Design tool angles (counter-clockwise) need conversion: -135deg in design = 225deg in Slint (clockwise from top).

## Semi-transparent Colors
- 8-digit hex with alpha (e.g., `#1d293d99`) requires `<brush>` type, not `<color>`.
- Slint `<color>` is opaque only; use `<brush>` for anything with alpha transparency.

## Font Weight
- `font-weight` uses integer values: 100-900. Common values: 400=normal, 600=semibold, 700=bold.
- Named values like "normal" or "bold" are NOT valid in Slint.

## Text Wrapping
- Use `wrap: word-wrap` (not `text-wrap` or `text-wrap: word-wrap`).

## GridLayout
- GridLayout uses `Row` sub-elements or explicit `row`/`col` properties on children.
- `for` and `if` are NOT allowed inside GridLayout.
- `spacing` applies uniformly to both rows and columns.

## Global Singletons
- Use `global Name { ... }` syntax (no `inherits` keyword needed).
- Access via `Name.property-name` in components.

## Imports
- Standard widgets: `import { Button, LineEdit } from "std-widgets.slint";`
- Only import what is actually used; unused imports may cause warnings.

## Images
- Use `@image-url("path")` syntax for Image source references.
- Do not set width/height on Image elements; retain original dimensions from the source file.
- Store all image assets in `assets/images/`.
- The type for image properties is lowercase `<image>`, NOT `<Image>`. `Image` is the element name, `image` is the type.

## Shadow Effects
- `drop-shadow-blur`, `drop-shadow-color`, `drop-shadow-offset-y` are set directly on Rectangle elements.
- NOT inside an `effect` block like in CSS or other frameworks.

## Window Configuration
- `default-font-family: "Inter"` sets the default font for all Text children in the Window.
- **CRITICAL**: `preferred-width` and `preferred-height` are NOT allowed when using `inherits Window`.
- When inheriting Window, use `width` and `height` directly on the root Rectangle inside the Window, or use a fixed-size root container.
- Correct pattern: `Rectangle { width: 448px; height: 900px; ... }` as the child of Window.

## Layout Alignment
- `alignment: center` on VerticalLayout/HorizontalLayout centers children on the cross axis.
- `alignment: space-between` distributes children evenly.
- `horizontal-stretch: 1` makes an element fill remaining horizontal space (acts like flex-grow).
- `vertical-stretch: 1` makes an element fill remaining vertical space.

## String Literals
- Unicode escape sequences (`\uXXXX`) are NOT parsed in Slint string literals.
- Always use actual UTF-8 characters (e.g., Korean text, emoji) directly in the source file.
- The file must be saved as UTF-8 encoding.
- NOTE: The Write tool converts non-ASCII characters (Korean, Chinese, emoji) to `\uXXXX` escape sequences, which Slint cannot parse. To fix this: write the file with the Write tool using `\uXXXX` escapes as placeholders, then use the Edit tool to replace each `\uXXXX` sequence with the actual UTF-8 character. The Edit tool preserves actual UTF-8 characters correctly.

## Component Background Gradients
- Background gradients set via `@linear-gradient()` are compile-time constants, NOT bindable properties.
- You cannot pass gradient colors as a property (e.g., `in property <[SomeType]> gradient-colors`).
- To reuse a component with different gradients, set `background` directly on each instance AFTER the component definition.
- The instance-level `background:` assignment overrides the component's default background.
- Example: `ActionButton { background: @linear-gradient(-135deg, #4f39f6ff 0%, #432dd7ff 100%); }`

## Image Export from .pen Files
- Use `mcp__pencil__export_nodes` with `scale: 1` to extract icon nodes as PNG images.
- Icons appear as `frame` nodes with child `path` elements in the .pen file structure.
- Rename exported files to descriptive names (e.g., `icon_qr.png`, `icon_call.png`) for clarity.
- Reference with `@image-url("assets/images/icon_name.png")` in Slint.

## Units
- All `<length>` values MUST have a unit suffix (e.g., `10px`, not `10`).
- This applies to `drop-shadow-offset-y`, `drop-shadow-blur`, etc. as well.
- Bare numbers without units cause "Cannot convert float to length" errors.

## Length Arithmetic (Centering Pattern)
- `(a - b) / 2` where a and b are `<length>` produces `length / float = length` -- this is VALID.
- `(a - b) / 2px` produces `float` (loses unit) -- this CAUSES "Cannot convert float to length" error.
- `(a - b) / 2 * 1px` produces `px * px = px^2` -- this CAUSES "Cannot convert px^2 to length" error.
- For centering, use: `x: parent.width / 2 - self.preferred-width / 2;` (length/float = length, then length - length = length).
- Alternative for fixed offsets: `x: parent.btn-width / 2 - 32px;` (both sides are length).

## No Margin Property
- Slint has no `margin-top`, `margin-bottom`, `margin-left`, `margin-right` properties.
- To create spacing between siblings, use the parent container's `spacing` property.
- To create spacing before the first child, use the parent container's `padding-top` etc.

## Text Padding
- `padding-*` on `Text` elements produces a compiler warning and has no visual effect.
- Text is not a layout container; use the parent's `spacing` or `gap` to control text position instead.

## Transparent Background
- `transparent` is NOT a valid color keyword in Slint.
- For transparent backgrounds on Rectangle elements, use `background: #00000000` instead.
- Alternatively, omit the `background` property entirely (defaults to transparent).

## Conditional Elements in Components
- `if condition: ElementType { ... }` is valid for conditionally showing/hiding child elements.
- Multiple `if` blocks on the same parent can show different content based on boolean properties.
- This pattern works well for icon-vs-text toggles in reusable button components.

## Icon Handling in Keypad Components
- Vector icons from .pen files should be exported as PNG images using `mcp__pencil__export_nodes`.
- Reference exported icons with `@image-url("assets/images/icon_name.png")`.
- For icons inside buttons, center using: `x: parent.width / 2 - icon_half_width;` (avoiding division by `2px`).
- The `<image>` type (lowercase) is the property type; `Image` (uppercase) is the element name.
