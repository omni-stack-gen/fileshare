---
name: Design Token Patterns
description: How to structure global design tokens for Slint projects
type: reference
---

# Design Token Patterns

## Token Organization
- Use a single `global AppTokens { ... }` block for all design tokens.
- Group tokens by category with comments: text, background, card, button.
- Export all tokens as `out property` for read-only access.

## Type Selection
- `<color>` for opaque solid colors (text, borders, solid backgrounds).
- `<brush>` for anything that might be a gradient OR semi-transparent color.
- `<length>` for spacing and font size values.
- `<string>` for font family names.

## Gradient Token Naming
- Name gradient tokens descriptively: `btn-qr-bg`, `btn-call-bg`, etc.
- Define gradient tokens inline in the global block using `@linear-gradient()`.
- This allows each button to accept a single `button-background` brush property.

## Smart Home Door Panel Tokens (Korean Intercom)
- Background: #0f172a (dark navy)
- Primary text: #ffffff, Muted text: #90a1b9
- Button gradients: various purple shades, guard button uses dark navy (#314158 -> #1d293d)
- Card backgrounds: semi-transparent (#1d293d99 weather, #1d293dcc visitor)
- Font: Inter, sizes range from 14px (labels) to 60px (clock)
