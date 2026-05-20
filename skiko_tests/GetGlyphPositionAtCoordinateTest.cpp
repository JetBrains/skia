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
    if (boxes.empty()) {
        return;
    }
    const SkRect& clusterRect = boxes[0].rect;
    SkScalar midX = (clusterRect.fLeft + clusterRect.fRight) / 2.0f;
    SkScalar midY = (clusterRect.fTop + clusterRect.fBottom) / 2.0f;

    auto position = paragraph->getGlyphPositionAtCoordinate(midX, midY).position;
    REPORTER_ASSERT(reporter, position != 2,
                    "mid-cluster click returned position %d (mid-grapheme)", position);
}
