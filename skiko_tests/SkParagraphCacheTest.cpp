#include "SkikoTest.h"

#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skparagraph/include/TextStyle.h"
#include "modules/skparagraph/src/ParagraphBuilderImpl.h"
#include "modules/skparagraph/utils/TestFontCollection.h"
#include "modules/skshaper/utils/FactoryHelpers.h"
#include "modules/skunicode/include/SkUnicode.h"
#include "tools/Resources.h"

using namespace skia::textlayout;

DEF_TEST_SKIKO(paragraphCache_unresolvedGlyphsConsistency, reporter) {
    auto factory = SkShapers::BestAvailable();
    sk_sp<SkUnicode> unicode = sk_ref_sp<SkUnicode>(factory->getUnicode());
    sk_sp<TestFontCollection> fontCollection =
            sk_make_sp<TestFontCollection>(GetResourcePath("fonts").c_str(), false, true);
    if (fontCollection->fontsFound() == 0) {
        ERRORF(reporter, "Skiko_paragraphCache_unresolvedGlyphsConsistency "
                         "requires fonts in resources/fonts/, none found");
        return;
    }

    const char* text1 = "Roboto ";  // Latin, fully resolved by Roboto
    const char* text2 = "Noto 是";   // 是 resolved by Noto Sans CJK JP
    const char* text3 = "字典";      // not covered by any font -> unresolved

    auto buildParagraph = [&](ParagraphStyle paragraph_style) {
        ParagraphBuilderImpl builder(paragraph_style, fontCollection, unicode);

        TextStyle text_style;
        text_style.setColor(SK_ColorBLACK);

        text_style.setFontFamilies({
                SkString("Not a real font"),
                SkString("Also a fake font"),
                SkString("Roboto"),
        });
        builder.pushStyle(text_style);
        builder.addText(text1, strlen(text1));

        text_style.setFontFamilies({
                SkString("Not a real font"),
                SkString("Roboto"),
                SkString("another fake one in between"),
                SkString("Noto Sans CJK JP"),
        });
        builder.pushStyle(text_style);
        builder.addText(text2, strlen(text2));

        text_style.setFontFamilies({
                SkString("So fake it is obvious"),
                SkString("Roboto"),
                SkString("Noto Sans CJK JP"),
        });
        builder.pushStyle(text_style);
        builder.addText(text3, strlen(text3));

        builder.pop();

        auto paragraph = builder.Build();
        paragraph->layout(1000);
        return paragraph;
    };

    ParagraphStyle paragraph_style;
    paragraph_style.turnHintingOff();

    auto paragraph = buildParagraph(paragraph_style);

    auto expectedUnresolvedGlyphs = paragraph->unresolvedGlyphs();
    auto expectedUnresolvedCodepoints = paragraph->unresolvedCodepoints();
    // 字 and 典 are the only unresolved glyphs/codepoints.
    REPORTER_ASSERT(reporter, expectedUnresolvedGlyphs == 2);
    REPORTER_ASSERT(reporter, expectedUnresolvedCodepoints.size() == 2);
    REPORTER_ASSERT(reporter, fontCollection->getParagraphCache()->count() == 1);

    // Building an identical paragraph must be a cache hit (the cache does not
    // grow) and must restore the unresolved glyphs/codepoints from the cache.
    auto paragraph2 = buildParagraph(paragraph_style);

    REPORTER_ASSERT(reporter, fontCollection->getParagraphCache()->count() == 1);
    REPORTER_ASSERT(reporter, paragraph2->unresolvedGlyphs() == expectedUnresolvedGlyphs);
    REPORTER_ASSERT(reporter, paragraph2->unresolvedCodepoints() == expectedUnresolvedCodepoints);
}
