#include "SkikoTest.h"

#include "include/core/SkScalar.h"
#include "modules/skparagraph/include/DartTypes.h"
#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skparagraph/include/TextStyle.h"
#include "modules/skparagraph/src/ParagraphBuilderImpl.h"
#include "modules/skparagraph/utils/TestFontCollection.h"
#include "modules/skshaper/utils/FactoryHelpers.h"
#include "modules/skunicode/include/SkUnicode.h"
#include "tools/Resources.h"

using namespace skia::textlayout;

// Click in the middle of a grapheme cluster formed by base + non-spacing mark.
// Without the snap-to-grapheme logic in
// ParagraphImpl::getGlyphPositionAtCoordinate, the hit-test returns the
// cluster's internal UTF-16 index (between the base and the combining mark),
// which is not a valid caret position. With the snap, the position is the
// cluster boundary.
//
// Text: "ba\u030Bc" -- 'b', 'a' followed by U+030B (combining double acute), 'c'
// rendered with Roboto. UTF-16 indices: b(0) a(1) acute(2) c(3); grapheme
// boundaries are 0, 1, 3, 4. Index 2 is mid-grapheme.
//
// The specific pair (a, U+030B) is chosen because Roboto Font has no precomposed
// glyph for it, so HarfBuzz emits two glyphs with separate cluster IDs and
// the hit-test can land on the mid-cluster index.
DEF_TEST_SKIKO(getGlyphPosition_combiningDoubleAcute_notMidGrapheme, reporter) {
    auto factory = SkShapers::BestAvailable();
    sk_sp<SkUnicode> unicode = sk_ref_sp<SkUnicode>(factory->getUnicode());
    sk_sp<TestFontCollection> fonts =
            sk_make_sp<TestFontCollection>(GetResourcePath("fonts").c_str(), false, true);
    if (fonts->fontsFound() == 0) {
        ERRORF(reporter, "Skiko_getGlyphPosition_combiningDoubleAcute_notMidGrapheme "
                         "requires fonts in resources/fonts/, none found");
        return;
    }

    ParagraphStyle paragraphStyle;
    TextStyle textStyle;
    textStyle.setFontFamilies({SkString("Roboto")});
    textStyle.setFontSize(50);
    textStyle.setColor(SK_ColorBLACK);

    const char* text = "ba\u030Bc";
    ParagraphBuilderImpl builder(paragraphStyle, fonts, unicode);
    builder.pushStyle(textStyle);
    builder.addText(text, strlen(text));
    builder.pop();
    auto paragraph = builder.Build();
    paragraph->layout(1000);

    auto boxes = paragraph->getRectsForRange(1, 3, RectHeightStyle::kTight, RectWidthStyle::kTight);
    REPORTER_ASSERT(reporter, !boxes.empty(),
                    "getRectsForRange returned no boxes for the target cluster");
    if (!boxes.empty()) {
        const SkRect& clusterRect = boxes[0].rect;
        SkScalar midX = (clusterRect.fLeft + clusterRect.fRight) / 2.0f;
        SkScalar midY = (clusterRect.fTop + clusterRect.fBottom) / 2.0f;

        auto position = paragraph->getGlyphPositionAtCoordinate(midX, midY).position;
        REPORTER_ASSERT(reporter, position != 2,
                        "mid-cluster click returned position %d (mid-grapheme)", position);
    }
}

// Tie-break: when the click x lands exactly on the geometric midpoint of a glyph,
// the two candidate caret positions (before-char and after-char) are equidistant.
// Android's Layout.getOffsetForHorizontal resolves this tie to the before-char side
// (Math.abs-distance comparison with strict `<` update keeps the earlier-encountered
// offset = the one to the visual left for LTR). Skia historically resolved it to
// after-char via `(dx < center) == leftToRight()`, which strictly fails at midpoint
// and falls through to the else branch.
//
// This test pins the Android-compatible behavior: clicking at the exact horizontal
// midpoint of glyph 'a' in "abc" yields offset 0 (before 'a'), not 1 (after 'a').
DEF_TEST_SKIKO(getGlyphPosition_midpointTieBreak_ltr_landsBeforeChar, reporter) {
    auto factory = SkShapers::BestAvailable();
    sk_sp<SkUnicode> unicode = sk_ref_sp<SkUnicode>(factory->getUnicode());
    sk_sp<TestFontCollection> fonts =
            sk_make_sp<TestFontCollection>(GetResourcePath("fonts").c_str(), false, true);
    if (fonts->fontsFound() == 0) {
        ERRORF(reporter, "Skiko_getGlyphPosition_midpointTieBreak_ltr_landsBeforeChar "
                         "requires fonts in resources/fonts/, none found");
        return;
    }

    ParagraphStyle paragraphStyle;
    TextStyle textStyle;
    textStyle.setFontFamilies({SkString("Roboto")});
    textStyle.setFontSize(50);
    textStyle.setColor(SK_ColorBLACK);

    const char* text = "abc";
    ParagraphBuilderImpl builder(paragraphStyle, fonts, unicode);
    builder.pushStyle(textStyle);
    builder.addText(text, strlen(text));
    builder.pop();
    auto paragraph = builder.Build();
    paragraph->layout(1000);

    // Locate the exact midpoint of glyph 'a' via getRectsForRange.
    auto boxes = paragraph->getRectsForRange(0, 1, RectHeightStyle::kTight, RectWidthStyle::kTight);
    REPORTER_ASSERT(reporter, !boxes.empty(),
                    "getRectsForRange returned no boxes for glyph 'a'");
    if (!boxes.empty()) {
        const SkRect& aRect = boxes[0].rect;
        SkScalar midX = (aRect.fLeft + aRect.fRight) / 2.0f;
        SkScalar midY = (aRect.fTop + aRect.fBottom) / 2.0f;

        auto position = paragraph->getGlyphPositionAtCoordinate(midX, midY).position;
        REPORTER_ASSERT(reporter, position == 0,
                        "midpoint click returned position %d, expected 0 (before-char)", position);
    }
}

// Companion regression test for the RTL side of midpoint tie-break: Skia historically
// returned the before-logical offset at the exact midpoint of an RTL glyph (via the
// original `(dx < center) == leftToRight()` evaluating to T == T → if-branch at midpoint),
// and the asymmetric fix preserves this behavior — only the LTR side is changed. This test
// pins the RTL outcome so future refactors don't accidentally flip it.
//
// Text "אבג" laid out as RTL; clicking the exact horizontal midpoint of the middle glyph
// ב (UTF-16 index 1) should yield offset 1 (before-ב logically = visually right of ב).
DEF_TEST_SKIKO(getGlyphPosition_midpointTieBreak_rtl_landsBeforeChar, reporter) {
    auto factory = SkShapers::BestAvailable();
    sk_sp<SkUnicode> unicode = sk_ref_sp<SkUnicode>(factory->getUnicode());
    sk_sp<TestFontCollection> fonts =
            sk_make_sp<TestFontCollection>(GetResourcePath("fonts").c_str(), false, true);
    if (fonts->fontsFound() == 0) {
        ERRORF(reporter, "Skiko_getGlyphPosition_midpointTieBreak_rtl_landsBeforeChar "
                         "requires fonts in resources/fonts/, none found");
        return;
    }

    ParagraphStyle paragraphStyle;
    paragraphStyle.setTextDirection(TextDirection::kRtl);
    TextStyle textStyle;
    textStyle.setFontFamilies({SkString("Roboto")});
    textStyle.setFontSize(50);
    textStyle.setColor(SK_ColorBLACK);

    // אבג — Hebrew Aleph, Bet, Gimel. Escaped to avoid relying on source-file encoding.
    const char* text = "אבג";
    ParagraphBuilderImpl builder(paragraphStyle, fonts, unicode);
    builder.pushStyle(textStyle);
    builder.addText(text, strlen(text));
    builder.pop();
    auto paragraph = builder.Build();
    paragraph->layout(1000);

    // Locate the exact midpoint of ב (the middle glyph, UTF-16 index 1).
    auto boxes = paragraph->getRectsForRange(1, 2, RectHeightStyle::kTight, RectWidthStyle::kTight);
    REPORTER_ASSERT(reporter, !boxes.empty(),
                    "getRectsForRange returned no boxes for ב");
    if (!boxes.empty()) {
        const SkRect& betRect = boxes[0].rect;
        SkScalar midX = (betRect.fLeft + betRect.fRight) / 2.0f;
        SkScalar midY = (betRect.fTop + betRect.fBottom) / 2.0f;

        auto position = paragraph->getGlyphPositionAtCoordinate(midX, midY).position;
        REPORTER_ASSERT(reporter, position == 1,
                        "RTL midpoint click returned position %d, expected 1 (before-logical)",
                        position);
    }
}

// BiDi-boundary hit-test: clicking inside an embedded opposite-direction run should
// snap to the cluster's paragraph-direction boundary, matching Android's
// closest-primary-direction-caret algorithm. Skia historically returned offsets pointing
// "inside" the embedded run via its run-internal RTL hit-test logic — this test pins
// the fix that prefers paragraph-direction boundaries.
//
// Text "aא." is laid out LTR (first-strong 'a'); א is a single-cluster RTL run embedded
// inside. Clicking on the LEFT visual half of א should yield offset 1 (= boundary
// between 'a' and א in paragraph reading direction), not offset 2 (= boundary inside
// the RTL run between א and '.'). The two offsets encode the same visual caret position
// but differ in which (offset, affinity) pair we return.
DEF_TEST_SKIKO(getGlyphPosition_bidiBoundary_embeddedRTL_snapsToParagraphDirection, reporter) {
    auto factory = SkShapers::BestAvailable();
    sk_sp<SkUnicode> unicode = sk_ref_sp<SkUnicode>(factory->getUnicode());
    sk_sp<TestFontCollection> fonts =
            sk_make_sp<TestFontCollection>(GetResourcePath("fonts").c_str(), false, true);
    if (fonts->fontsFound() == 0) {
        ERRORF(reporter,
               "Skiko_getGlyphPosition_bidiBoundary_embeddedRTL_snapsToParagraphDirection "
               "requires fonts in resources/fonts/, none found");
        return;
    }

    ParagraphStyle paragraphStyle;
    paragraphStyle.setTextDirection(TextDirection::kLtr);
    TextStyle textStyle;
    textStyle.setFontFamilies({SkString("Roboto")});
    textStyle.setFontSize(50);
    textStyle.setColor(SK_ColorBLACK);

    // "aא." — 'a' (LTR), Aleph (RTL embedded), '.' (neutral). UTF-16 indices: a(0), א(1), .(2).
    const char* text = "aא.";
    ParagraphBuilderImpl builder(paragraphStyle, fonts, unicode);
    builder.pushStyle(textStyle);
    builder.addText(text, strlen(text));
    builder.pop();
    auto paragraph = builder.Build();
    paragraph->layout(1000);

    // Get the visual rect of א and probe its visual halves.
    auto boxes = paragraph->getRectsForRange(1, 2, RectHeightStyle::kTight, RectWidthStyle::kTight);
    REPORTER_ASSERT(reporter, !boxes.empty(),
                    "getRectsForRange returned no boxes for א");
    if (!boxes.empty()) {
        const SkRect& alephRect = boxes[0].rect;
        SkScalar midY = (alephRect.fTop + alephRect.fBottom) / 2.0f;
        SkScalar quarterWidth = (alephRect.fRight - alephRect.fLeft) / 4.0f;

        // Click on visual LEFT half (1px inside the left edge → paragraph-before of α).
        SkScalar leftQuarter = alephRect.fLeft + quarterWidth;
        auto leftPos = paragraph->getGlyphPositionAtCoordinate(leftQuarter, midY).position;
        REPORTER_ASSERT(reporter, leftPos == 1,
                        "left-half click in embedded RTL returned position %d, expected 1 "
                        "(paragraph-direction before α)", leftPos);

        // Click on visual RIGHT half → paragraph-after of α.
        SkScalar rightQuarter = alephRect.fLeft + 3 * quarterWidth;
        auto rightPos = paragraph->getGlyphPositionAtCoordinate(rightQuarter, midY).position;
        REPORTER_ASSERT(reporter, rightPos == 2,
                        "right-half click in embedded RTL returned position %d, expected 2 "
                        "(paragraph-direction after α)", rightPos);
    }
}
