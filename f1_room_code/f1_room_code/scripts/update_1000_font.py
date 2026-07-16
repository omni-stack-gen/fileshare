#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
import unicodedata
from pathlib import Path

from fontTools.pens.recordingPen import DecomposingRecordingPen
from fontTools.pens.transformPen import TransformPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont


ROOT = Path(__file__).resolve().parents[1]
TARGET_FONT = ROOT / "1000.TTF"
FALLBACK_FONTS = [
    ROOT / "NotoSansJP-Regular.ttf",
    ROOT / "NotoSansKR-Regular.ttf",
]
REPORT_PATH = ROOT / "font-coverage.md"


def normalized_chars(text: str) -> set[str]:
    chars = set()
    for char in unicodedata.normalize("NFC", text):
        if char in {"\n", "\r", "\t"}:
            continue
        chars.add(char)
    return chars


def quoted_strings(text: str) -> list[str]:
    return re.findall(r'"((?:[^"\\]|\\.)*)"', text)


def unescape_string(value: str) -> str:
    if "\\" not in value:
        return value
    return bytes(value, "utf-8").decode("unicode_escape")


def collect_ui_strings() -> set[str]:
    strings: set[str] = set()

    property_pattern = re.compile(r"(?:text|label|title)\s*:\s*\"((?:[^\"\\]|\\.)*)\"")
    value_pattern = re.compile(
        r"(?:connected-ssid|wifi-target-ssid|general-year|general-month|general-day|"
        r"general-hour|general-minute|time-period|absent-mode-time|communication-time|"
        r"second-level-value)\s*[:=]\s*\"((?:[^\"\\]|\\.)*)\""
    )

    for path in sorted((ROOT / "ui").glob("*.slint")):
        for line in path.read_text(encoding="utf-8").splitlines():
            if "return" in line:
                strings.update(quoted_strings(line))
            strings.update(property_pattern.findall(line))
            strings.update(value_pattern.findall(line))

    for path in [ROOT / "src" / "main.rs", ROOT / "src" / "db.rs"]:
        if not path.exists():
            continue
        for line in path.read_text(encoding="utf-8").splitlines():
            if "get_string(" in line or "connected_ssid" in line or "HomeNetwork_5G" in line:
                strings.update(quoted_strings(line))

    return {unescape_string(value) for value in strings}


def collect_required_chars() -> set[str]:
    required = set()
    for text in collect_ui_strings():
        required.update(normalized_chars(text))
    return required


def font_codepoints(path: Path) -> set[int]:
    with TTFont(path) as font:
        codepoints: set[int] = set()
        for table in font["cmap"].tables:
            codepoints.update(table.cmap.keys())
        return codepoints


def unique_glyph_name(font: TTFont, codepoint: int, preferred_name: str) -> str:
    existing = set(font.getGlyphOrder())
    if preferred_name not in existing:
        return preferred_name

    fallback_name = f"uni{codepoint:04X}"
    if fallback_name not in existing:
        return fallback_name

    suffix = 1
    while True:
        candidate = f"{fallback_name}.{suffix}"
        if candidate not in existing:
            return candidate
        suffix += 1


def unicode_cmap_tables(font: TTFont):
    for table in font["cmap"].tables:
        if table.isUnicode():
            yield table


def import_missing_chars(target: Path, fallback: Path, missing_codepoints: set[int]) -> None:
    if not missing_codepoints:
        return

    target_font = TTFont(target)
    fallback_font = TTFont(fallback)

    target_upm = target_font["head"].unitsPerEm
    fallback_upm = fallback_font["head"].unitsPerEm
    scale = target_upm / fallback_upm

    target_glyph_order = target_font.getGlyphOrder()
    target_glyf = target_font["glyf"]
    target_hmtx = target_font["hmtx"].metrics
    target_cmap_tables = list(unicode_cmap_tables(target_font))

    fallback_best_cmap = fallback_font.getBestCmap()
    fallback_glyph_set = fallback_font.getGlyphSet()
    fallback_hmtx = fallback_font["hmtx"].metrics

    for codepoint in sorted(missing_codepoints):
        fallback_glyph_name = fallback_best_cmap.get(codepoint)
        if not fallback_glyph_name:
            continue

        recording_pen = DecomposingRecordingPen(fallback_glyph_set)
        fallback_glyph_set[fallback_glyph_name].draw(recording_pen)

        glyph_pen = TTGlyphPen(None)
        transform_pen = TransformPen(glyph_pen, (scale, 0, 0, scale, 0, 0))
        recording_pen.replay(transform_pen)
        glyph = glyph_pen.glyph()

        new_glyph_name = unique_glyph_name(target_font, codepoint, fallback_glyph_name)
        advance_width, left_side_bearing = fallback_hmtx[fallback_glyph_name]

        target_glyph_order.append(new_glyph_name)
        target_glyf[new_glyph_name] = glyph
        target_hmtx[new_glyph_name] = (
            round(advance_width * scale),
            round(left_side_bearing * scale),
        )

        for table in target_cmap_tables:
            if codepoint > 0xFFFF and table.format == 4:
                continue
            table.cmap[codepoint] = new_glyph_name

    target_font.setGlyphOrder(target_glyph_order)
    target_font["maxp"].numGlyphs = len(target_glyph_order)
    target_font["hhea"].numberOfHMetrics = len(target_hmtx)

    temp_output = target.with_suffix(".tmp.ttf")
    target_font.save(temp_output)
    temp_output.replace(target)



def describe_char(char: str) -> str:
    codepoint = f"U+{ord(char):04X}"
    name = unicodedata.name(char, "UNKNOWN")
    visible = char
    if char == " ":
        visible = "<space>"
    return f"- `{visible}` {codepoint} {name}"


def write_report(
    required_chars: set[str],
    added_by_source: dict[str, set[str]],
    missing_in_both: set[str],
    target_before: set[int],
    target_after: set[int],
    fallback_codepoints: dict[str, set[int]],
    report_path: Path,
) -> None:
    required_no_space = {char for char in required_chars if char != " "}
    covered_before = {char for char in required_chars if ord(char) in target_before}
    added_total = set().union(*added_by_source.values()) if added_by_source else set()

    lines = [
        "# Font Coverage Report",
        "",
        f"- Target font: `{TARGET_FONT.name}`",
        "- Fallback fonts:",
    ]
    lines.extend(f"  - `{path.name}`" for path in FALLBACK_FONTS if path.exists())
    lines.extend([
        "",
        f"- Unique required characters: `{len(required_no_space)}` visible chars",
        f"- Covered by `1000.TTF` before merge: `{len(covered_before - {' '})}`",
        f"- Added from fallback fonts: `{len(added_total)}`",
        f"- Still missing in both fonts: `{len(missing_in_both)}`",
        f"- Total codepoints in `1000.TTF` before merge: `{len(target_before)}`",
        f"- Total codepoints in `1000.TTF` after merge: `{len(target_after)}`",
        "",
        "## Added From Fallback Fonts",
        "",
    ])

    if added_by_source:
        for font_name, chars in added_by_source.items():
            lines.append(f"### {font_name}")
            lines.append("")
            lines.append(f"- Imported chars: `{len(chars)}`")
            lines.append(f"- Total codepoints in source font: `{len(fallback_codepoints[font_name])}`")
            lines.append("")
            if chars:
                lines.extend(describe_char(char) for char in sorted(chars, key=ord))
            else:
                lines.append("- None")
            lines.append("")
    else:
        lines.append("- None")

    lines.extend(["", "## Missing In Both Fonts", ""])

    if missing_in_both:
        lines.extend(describe_char(char) for char in sorted(missing_in_both, key=ord))
    else:
        lines.append("- None")

    lines.extend(["", "## Required Character Set", "", "```text"])
    lines.append("".join(sorted(required_no_space, key=ord)))
    lines.extend(["```", ""])

    report_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--report", type=Path, default=REPORT_PATH)
    args = parser.parse_args()
    report_path = args.report

    required_chars = collect_required_chars()
    target_before = font_codepoints(TARGET_FONT)
    fallback_codepoints: dict[str, set[int]] = {}
    added_by_source: dict[str, set[str]] = {}

    remaining_missing = {char for char in required_chars if ord(char) not in target_before}

    for fallback_font in FALLBACK_FONTS:
        if not fallback_font.exists():
            continue
        available_codepoints = font_codepoints(fallback_font)
        fallback_codepoints[fallback_font.name] = available_codepoints
        available_chars = {char for char in remaining_missing if ord(char) in available_codepoints}
        if available_chars:
            import_missing_chars(TARGET_FONT, fallback_font, {ord(char) for char in available_chars})
        added_by_source[fallback_font.name] = available_chars
        remaining_missing -= available_chars

    missing_in_both = remaining_missing

    target_after = font_codepoints(TARGET_FONT)
    write_report(
        required_chars=required_chars,
        added_by_source=added_by_source,
        missing_in_both=missing_in_both,
        target_before=target_before,
        target_after=target_after,
        fallback_codepoints=fallback_codepoints,
        report_path=report_path,
    )

    imported_chars = set().union(*added_by_source.values()) if added_by_source else set()
    unresolved_after_merge = {char for char in imported_chars if ord(char) not in target_after}
    if unresolved_after_merge:
        missing = ", ".join(f"U+{ord(char):04X}" for char in sorted(unresolved_after_merge, key=ord))
        raise SystemExit(f"Merge completed but target font still misses: {missing}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
