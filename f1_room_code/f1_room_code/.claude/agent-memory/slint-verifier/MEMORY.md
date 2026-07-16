# Slint Verifier Agent Memory

## Verification Patterns

### Text Alignment Best Practices
- **Issue**: Design reports contain relative x-offsets (e.g., 6.26px, 4.38px) that should NOT be used for absolute positioning
- **Solution**: Always use `x: (parent.width - self.width) / 2` combined with `horizontal-alignment: center` for center-aligned text
- **Why**: Text elements in designs are often positioned with small offsets due to font metrics, but in Slint we want flexible centering

### Position Tolerance Thresholds
- Position deviation: ≤ 20px acceptable (due to rendering differences)
- Color deviation: Near-match acceptable (image analysis approximates colors)
- Font size: Exact match required
- Text truncation: Zero tolerance

### Common Rendering Issues
1. **Text not centered**: Use explicit x calculation + horizontal-alignment
2. **Icons not loading**: Verify `@image-url` paths are relative from ui/ directory
3. **Colors slightly off**: Image analysis may approximate; verify hex values in code

## Icon Asset Verification
Always verify these assets exist before rendering:
- `assets/images/1g9pu.png` - Door icon
- `assets/images/m4jYU.png` - Checkmark icon  
- `assets/images/cZfHB.png` - Bottom decorative icon

## Slint Compilation
Always run `/usr/bin/slint-compiler -o /dev/null [file]` before finalizing.

## Image Analysis Notes
- Rendered images are scaled (e.g., 448x900 -> 500x1000)
- Position calculations must account for scaling factor
- Color analysis is approximate; trust hex values in code
