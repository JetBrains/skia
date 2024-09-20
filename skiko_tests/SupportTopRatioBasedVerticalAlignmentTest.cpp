#include "SkikoTest.h"
#include "modules/skparagraph/include/TextStyle.h"
#include "modules/skparagraph/include/ParagraphStyle.h"

DEF_TEST_SKIKO(TextStyle_topRatio, reporter) {
    skia::textlayout::TextStyle ts = skia::textlayout::TextStyle();
    REPORTER_ASSERT(reporter, ts.getTopRatio() == -1.0f);
    REPORTER_ASSERT(reporter, ts.getHalfLeading() == false);

    ts.setTopRatio(0.12345f);
    REPORTER_ASSERT(reporter, ts.getTopRatio() == 0.12345f);
    REPORTER_ASSERT(reporter, ts.getHalfLeading() == false);

    ts.setHalfLeading(true);
    REPORTER_ASSERT(reporter, ts.getTopRatio() == 0.5f);
    REPORTER_ASSERT(reporter, ts.getHalfLeading() == true);

    ts.setHalfLeading(false);
    REPORTER_ASSERT(reporter, ts.getTopRatio() == -1.0f);
    REPORTER_ASSERT(reporter, ts.getHalfLeading() == false);

    ts.setTopRatio(0.98765f);
    skia::textlayout::TextStyle clone = ts.cloneForPlaceholder();
    REPORTER_ASSERT(reporter, clone.getTopRatio() == 0.98765f);
}

DEF_TEST_SKIKO(StrutStyle_topRatio, reporter) {
    skia::textlayout::StrutStyle ss = skia::textlayout::StrutStyle();
    REPORTER_ASSERT(reporter, ss.getTopRatio() == -1.0f);
    REPORTER_ASSERT(reporter, ss.getHalfLeading() == false);

    ss.setTopRatio(0.12345f);
    REPORTER_ASSERT(reporter, ss.getTopRatio() == 0.12345f);
    REPORTER_ASSERT(reporter, ss.getHalfLeading() == false);

    ss.setHalfLeading(true);
    REPORTER_ASSERT(reporter, ss.getTopRatio() == 0.5f);
    REPORTER_ASSERT(reporter, ss.getHalfLeading() == true);

    ss.setHalfLeading(false);
    REPORTER_ASSERT(reporter, ss.getTopRatio() == -1.0f);
    REPORTER_ASSERT(reporter, ss.getHalfLeading() == false);
}