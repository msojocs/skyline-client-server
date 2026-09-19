#!/usr/bin/env python3
"""Build the CJK/symbol/colour-emoji font used by the Skyline Wine launcher.

Requires fonttools==4.65.0. Uses Noto Color Emoji bitmaps, Droid Sans Fallback text, and Symbola
symbols. Segoe contributes only five legacy symbols, never emoji artwork.
"""

import argparse
import hashlib
import os
import tempfile
from copy import deepcopy
from pathlib import Path

from fontTools import subset
from fontTools.ttLib.tables.sbixStrike import Strike
from fontTools.ttLib.tables.sbixGlyph import Glyph
from fontTools.merge import Merger
from fontTools.pens.recordingPen import DecomposingRecordingPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont, newTable
from fontTools.ttLib.tables._c_m_a_p import CmapSubtable
from fontTools.ttLib.reorderGlyphs import reorderGlyphs
from fontTools.ttLib.scaleUpem import scale_upem


RESOURCE_DIR = Path(__file__).resolve().parent
SOURCE_HASHES = {
    "text": "acb6440a713d880a13a21b468ba7cd43f5a2b2934972e51be791c880730777b8",
    "symbols": "af8bd8cc9c808272034651cb5ebf6c38370ae536fe631eb89462f127f11e793f",
    "emoji": "9fd0a3d0ce84d77e3185dfbae77bd1abf3926aa49a032e354d076c4f17151f10",
    "legacy": "f07cbd7886f4a1a5255a1bdf4ca5ea29db3be3357414003a92bec5d1c3165578",
}
OUTLINE_TABLES = {
    "GlyphOrder", "head", "hhea", "maxp", "hmtx", "cmap", "name", "OS/2",
    "post", "glyf", "loca", "fpgm", "prep", "cvt ", "gasp",
}


def checked_font(path, kind):
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest != SOURCE_HASHES[kind]:
        raise ValueError(f"Unexpected {kind} font: {path} ({digest})")
    return TTFont(path, recalcTimestamp=False)


def make_base(text, symbols):
    """Reproduce the previous monochrome font without using a cached build."""
    with tempfile.TemporaryDirectory(prefix="skyline-font-build-") as directory:
        paths = []
        for index, font in enumerate((text, symbols)):
            if font["head"].unitsPerEm != 2048:
                scale_upem(font, 2048)
            for tag in list(font.keys()):
                if tag not in OUTLINE_TABLES:
                    del font[tag]
            path = Path(directory) / f"outline-{index}.ttf"
            font.save(path)
            paths.append(str(path))
        return Merger().merge(paths)


def keep_reachable_glyphs(font):
    options = subset.Options()
    options.glyph_names = True
    options.name_IDs = ["*"]
    options.layout_features = ["*"]
    worker = subset.Subsetter(options=options)
    worker.populate(unicodes=font.getBestCmap())
    worker.subset(font)


def flatten_components(font):
    """Free component-only glyph slots while keeping every encoded character.

    Flattening the CJK/symbol outlines lets unused component glyphs be
    removed before bitmap glyphs are added, without reducing Unicode coverage.
    """
    glyph_set = font.getGlyphSet()
    replacements = {}
    for name in font.getGlyphOrder():
        if font["glyf"][name].isComposite():
            recording = DecomposingRecordingPen(glyph_set)
            glyph_set[name].draw(recording)
            pen = TTGlyphPen(None)
            recording.replay(pen)
            replacements[name] = pen.glyph()
    for name, glyph in replacements.items():
        font["glyf"][name] = glyph
    keep_reachable_glyphs(font)


def remap_layout(value, names):
    """Rewrite glyph references, including coverage and ligature dictionaries."""
    if isinstance(value, str):
        return names.get(value, value)
    if isinstance(value, list):
        return [remap_layout(item, names) for item in value]
    if isinstance(value, tuple):
        return tuple(remap_layout(item, names) for item in value)
    if isinstance(value, dict):
        return {remap_layout(key, names): remap_layout(item, names)
                for key, item in value.items()}
    if hasattr(value, "__dict__"):
        for key, item in list(vars(value).items()):
            setattr(value, key, remap_layout(item, names))
    return value


def add_legacy_symbols(base, legacy):
    """Keep five non-emoji symbols that existed in the previous font."""
    wanted = {0x3244, 0x3245, 0x3246, 0x3247, 0xF8FF}
    source = legacy.getBestCmap()
    names = {}

    def copy_glyph(name):
        if name in names:
            return names[name]
        target = "legacy." + name
        names[name] = target
        glyph = deepcopy(legacy["glyf"][name])
        if glyph.isComposite():
            for part in glyph.components:
                part.glyphName = copy_glyph(part.glyphName)
        base["glyf"][target] = glyph
        base["hmtx"][target] = legacy["hmtx"][name]
        return target

    for codepoint in wanted - set(base.getBestCmap()):
        glyph = copy_glyph(source[codepoint])
        for table in base["cmap"].tables:
            if table.isUnicode():
                table.cmap[codepoint] = glyph
    base.setGlyphOrder(list(base["glyf"].glyphOrder))


def add_colour(base, emoji):
    """Embed Noto's original PNGs in sbix; no tracing or palette reduction."""
    required_codepoints = set(base.getBestCmap()) | set(emoji.getBestCmap())
    flatten_components(base)
    base_map, emoji_map = base.getBestCmap(), emoji.getBestCmap()
    names = {name: "noto." + name for name in emoji.getGlyphOrder()}
    for codepoint, name in emoji_map.items():
        if codepoint < 128 and codepoint in base_map:
            # Ordinary spaces/digits stay text; GSUB can still form keycaps.
            names[name] = base_map[codepoint]
    for name in emoji.getGlyphOrder():
        target = names[name]
        if target not in base["glyf"]:
            base["glyf"][target] = TTGlyphPen(None).glyph()
            base["hmtx"][target] = emoji["hmtx"][name]
    for table in base["cmap"].tables:
        if table.isUnicode():
            for codepoint, name in emoji_map.items():
                if table.format in (12, 13) or codepoint <= 0xFFFF:
                    table.cmap[codepoint] = names[name]
    for table in emoji["cmap"].tables:
        if table.format == 14:
            variation = CmapSubtable.newSubtable(14)
            variation.platformID = table.platformID
            variation.platEncID = table.platEncID
            variation.language = table.language
            variation.cmap = {}
            variation.uvsDict = {
                selector: [(codepoint, names[name] if name else None)
                           for codepoint, name in mappings]
                for selector, mappings in table.uvsDict.items()
            }
            base["cmap"].tables.append(variation)
    base.setGlyphOrder(list(base["glyf"].glyphOrder))
    if len(base.getGlyphOrder()) > 65535:
        raise ValueError("Combined font exceeds TrueType's glyph limit")

    base["sbix"] = newTable("sbix")
    base["sbix"].strikes = {}
    for index, bitmaps in enumerate(emoji["CBDT"].strikeData):
        size = emoji["CBLC"].strikes[index].bitmapSizeTable
        if size.ppemX != size.ppemY:
            raise ValueError("Non-square bitmap resolution is not supported")
        strike = Strike(ppem=size.ppemY)
        for name, bitmap in bitmaps.items():
            if not names[name].startswith("noto."):
                continue
            bitmap.ensureDecompiled()
            metrics = bitmap.metrics
            strike.glyphs[names[name]] = Glyph(
                glyphName=names[name], graphicType="png ",
                originOffsetX=metrics.BearingX,
                originOffsetY=metrics.BearingY - metrics.height,
                imageData=bitmap.imageData,
            )
        base["sbix"].strikes[strike.ppem] = strike
    for tag in ("GSUB", "GDEF"):
        if tag in emoji:
            base[tag] = remap_layout(deepcopy(emoji[tag]), names)
    reorderGlyphs(base, base.getGlyphOrder())

    for name in base.getGlyphOrder():
        base["glyf"][name].removeHinting()
    for tag in ("fpgm", "prep", "cvt "):
        if tag in base:
            del base[tag]
    for record in base["name"].names:
        name = None
        if record.nameID in (1, 4, 16):
            name = "Skyline Fallback"
        elif record.nameID == 6:
            name = "SkylineFallback"
        elif record.nameID in (2, 17):
            name = "Regular"
        elif record.nameID == 5:
            name = "Version 1.200; Noto colour emoji 2.051"
        if name is not None:
            record.string = name.encode(record.getEncoding())
    base.recalcTimestamp = False
    base["head"].created = base["head"].modified = (
        int(os.environ.get("SOURCE_DATE_EPOCH", "1789776000")) + 2082844800
    )
    base["head"].fontRevision = 1.2
    if required_codepoints - set(base.getBestCmap()):
        raise ValueError("Font generation lost Unicode coverage")
    for char in "🚜🍎😄":
        source_name = emoji_map[ord(char)]
        name = base.getBestCmap()[ord(char)]
        for index, size in enumerate(emoji["CBLC"].strikes):
            bitmap = emoji["CBDT"].strikeData[index][source_name]
            if base["sbix"].strikes[size.bitmapSizeTable.ppemY].glyphs[name].imageData != bitmap.imageData:
                raise ValueError(f"Noto bitmap changed for {char}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--text-font", type=Path, default=Path(
        "/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf"))
    parser.add_argument("--output", type=Path,
                        default=RESOURCE_DIR / "SkylineFallback.ttf")
    args = parser.parse_args()
    text = checked_font(args.text_font, "text")
    symbols = checked_font(RESOURCE_DIR / "Symbola.ttf", "symbols")
    emoji = checked_font(RESOURCE_DIR / "NotoColorEmoji.ttf", "emoji")
    legacy = checked_font(RESOURCE_DIR / "seguiemj.ttf", "legacy")
    base = make_base(text, symbols)
    add_legacy_symbols(base, legacy)
    add_colour(base, emoji)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    # Never truncate a font that a running Wine process may have mapped.
    with tempfile.NamedTemporaryFile(dir=args.output.parent, suffix=".ttf",
                                     delete=False) as stream:
        temporary = Path(stream.name)
    try:
        base.save(temporary)
        temporary.chmod(0o644)
        temporary.replace(args.output)
    finally:
        temporary.unlink(missing_ok=True)
    digest = hashlib.sha256(args.output.read_bytes()).hexdigest()
    print(f"{args.output}: {len(base.getGlyphOrder())} glyphs, "
          f"{len(base.getBestCmap())} codepoints, "
          f"{sum(len(s.glyphs) for s in base['sbix'].strikes.values())} bitmap glyphs\nsha256: {digest}")


if __name__ == "__main__":
    main()
