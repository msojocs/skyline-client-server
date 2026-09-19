/* Build with x86_64-w64-mingw32-gcc bitmap-probe.c -ldwrite -lole32.
 * Run inside the same Wine module overlay as Skyline. Optional first argument:
 * output path for the returned tractor PNG (for byte-for-byte comparison).
 */
#define COBJMACROS
#include <initguid.h>
#include <windows.h>
#include <dwrite_3.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(int argc, char **argv)
{
    IDWriteFactory2 *factory;
    IDWriteFontCollection *collection;
    IDWriteFontFamily *family;
    IDWriteFont *font;
    IDWriteFontFace *face;
    IDWriteFontFace4 *face4;
    IDWriteFontFace2 *face2;
    UINT32 index, codepoints[] = { 0x62d6, 0x61, 0x31, 0x2228, 0x1f69c, 0x1f34e, 0x1f604 };
    UINT16 glyphs[7], invalid;
    DWRITE_GLYPH_METRICS metrics[7];
    DWRITE_GLYPH_IMAGE_DATA image;
    DWRITE_GLYPH_IMAGE_FORMATS formats;
    BOOL exists;
    void *context;
    HRESULT hr;
    unsigned int i;
    FILE *output;

    CHECK(SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory2,
                                       (IUnknown **)&factory)));
    CHECK(SUCCEEDED(IDWriteFactory2_GetSystemFontCollection(factory, &collection, FALSE)));
    CHECK(SUCCEEDED(IDWriteFontCollection_FindFamilyName(collection, L"Skyline Fallback", &index, &exists)) && exists);
    CHECK(SUCCEEDED(IDWriteFontCollection_GetFontFamily(collection, index, &family)));
    CHECK(SUCCEEDED(IDWriteFontFamily_GetFirstMatchingFont(family, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font)));
    CHECK(SUCCEEDED(IDWriteFont_CreateFontFace(font, &face)));
    CHECK(SUCCEEDED(IDWriteFontFace_QueryInterface(face, &IID_IDWriteFontFace4, (void **)&face4)));
    CHECK(SUCCEEDED(IDWriteFontFace_QueryInterface(face, &IID_IDWriteFontFace2, (void **)&face2)));
    CHECK(IDWriteFontFace2_IsColorFont(face2));
    CHECK(SUCCEEDED(IDWriteFontFace_GetGlyphIndices(face, codepoints, 7, glyphs)));
    CHECK(SUCCEEDED(IDWriteFontFace_GetDesignGlyphMetrics(face, glyphs, 7, metrics, FALSE)));
    for (i = 0; i < 7; ++i)
    {
        CHECK(glyphs[i] != 0 && metrics[i].advanceWidth > 0);
        CHECK(SUCCEEDED(IDWriteFontFace4_GetGlyphImageFormats_(face4, glyphs[i], 0, ~0u, &formats)));
        if (i < 4)
        {
            CHECK(!(formats & DWRITE_GLYPH_IMAGE_FORMATS_PNG));
            CHECK(formats & DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE);
            continue;
        }
        CHECK(formats & DWRITE_GLYPH_IMAGE_FORMATS_PNG);
        CHECK(SUCCEEDED(IDWriteFontFace4_GetGlyphImageData(face4, glyphs[i], 13,
                DWRITE_GLYPH_IMAGE_FORMATS_PNG, &image, &context)));
        CHECK(context && image.imageDataSize > 33 && image.pixelsPerEm == 109);
        CHECK(image.pixelSize.width > 0 && image.pixelSize.height == 128);
        CHECK(!memcmp(image.imageData, "\x89PNG\r\n\x1a\n", 8));
        CHECK(image.horizontalLeftOrigin.y == 101);
        CHECK(image.horizontalRightOrigin.x > image.horizontalLeftOrigin.x);
        printf("U+%X: %ux%u pixels, ppem %u, %u bytes, origin (%ld,%ld)\n", codepoints[i],
               image.pixelSize.width, image.pixelSize.height, image.pixelsPerEm,
               image.imageDataSize, image.horizontalLeftOrigin.x, image.horizontalLeftOrigin.y);
        if (i == 4 && argc > 1)
        {
            CHECK((output = fopen(argv[1], "wb")) != NULL);
            CHECK(fwrite(image.imageData, 1, image.imageDataSize, output) == image.imageDataSize);
            CHECK(!fclose(output));
        }
        IDWriteFontFace4_ReleaseGlyphImageData(face4, context);
    }
    /* Repeated calls must keep/release the image table safely. */
    for (i = 0; i < 100; ++i)
    {
        CHECK(SUCCEEDED(IDWriteFontFace4_GetGlyphImageData(face4, glyphs[4], 64,
                DWRITE_GLYPH_IMAGE_FORMATS_PNG, &image, &context)));
        IDWriteFontFace4_ReleaseGlyphImageData(face4, context);
    }
    invalid = IDWriteFontFace_GetGlyphCount(face);
    CHECK(IDWriteFontFace4_GetGlyphImageFormats_(face4, invalid, 0, ~0u, &formats) == E_INVALIDARG);
    CHECK(IDWriteFontFace4_GetGlyphImageFormats_(face4, glyphs[4], 2, 1, &formats) == E_INVALIDARG);
    CHECK(IDWriteFontFace4_GetGlyphImageFormats_(face4, glyphs[4], 0, ~0u, NULL) == E_INVALIDARG);
    CHECK(IDWriteFontFace4_GetGlyphImageData(face4, invalid, 13,
            DWRITE_GLYPH_IMAGE_FORMATS_PNG, &image, &context) == E_INVALIDARG);
    hr = IDWriteFontFace4_GetGlyphImageData(face4, glyphs[0], 13,
            DWRITE_GLYPH_IMAGE_FORMATS_PNG, &image, &context);
    CHECK(hr == DWRITE_E_NOCOLOR && !context && !image.imageData);
    IDWriteFontFace2_Release(face2);
    IDWriteFontFace4_Release(face4);
    IDWriteFontFace_Release(face);
    IDWriteFont_Release(font);
    IDWriteFontFamily_Release(family);
    IDWriteFontCollection_Release(collection);
    IDWriteFactory2_Release(factory);
    puts("PASS: text metrics, colour PNGs, repeated release, invalid glyphs and arguments");
    return 0;
}
