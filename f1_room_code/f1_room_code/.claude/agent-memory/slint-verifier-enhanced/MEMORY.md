# Enhanced Slint Verifier Memory

## Verification Patterns

### Icon Size Handling
When design specifies an icon with clipping (e.g., 90x90px in a 52px container):
- Set Image width/height to the full asset size (90px)
- Use negative x/y offsets to center: `(container_size - asset_size) / 2`
- Example: `x: -19px; y: -19px;` for 90px icon in 52px space

### Container Width Precision
- Always match exact design widths, even if text seems to fit
- Common issue: Using wider containers (120px) than design (83px)
- Verify parent container constraints when adjusting widths

### Text Element Positioning
- Korean text may need slightly different positioning than Latin text
- Use `vertical-alignment: center` for single-line text
- For multi-line or complex layouts, explicit y positions are acceptable

## Common Issues Found

| Issue | Frequency | Severity | Fix Strategy |
|-------|-----------|----------|--------------|
| Icon size mismatch | High | Medium | Use design asset size with offset |
| Container width | Medium | Low | Match design exactly |
| Missing dual shadows | Medium | Low | Slint limitation, document in report |
| Absolute positioning | Low | Low | Acceptable if visually balanced |

## Verification Thresholds

- **Position deviation:** <= 15px acceptable
- **Size deviation:** <= 5px acceptable
- **Color:** Must match exactly (hex + alpha)
- **Typography:** Font size/weight must match exactly

## Files Generated

- Verification reports: `.claude/agent-memory/slint-verification/[NodeID]-enhanced.md`
- Rendered screenshots: `slint-screenshots/[NodeID]-enhanced-v[N].png`
- Fix scripts: `.claude/agent-memory/slint-verifier-enhanced/verify_[NodeID].sh`

## Iteration Workflow

1. Read layout report for design specifications
2. Read Slint source code
3. Compare code values to design values
4. Identify deviations > threshold
5. Apply fixes to Slint code
6. Re-verify (up to 3 iterations)
7. Generate final report

## Report Structure

Reports must include:
1. Executive summary with status
2. Pixel-level comparison tables
3. Fix history with before/after code
4. Remaining issues (with priority)
5. Files referenced
