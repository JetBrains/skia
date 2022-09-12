// Copyright 2019 Google LLC.
#ifndef FontRastrSettings_DEFINED
#define FontRastrSettings_DEFINED

#include "include/core/SkFont.h"

namespace skia {
namespace textlayout {

struct FontRastrSettings {
    FontRastrSettings() {
        fEdging = SkFont::Edging::kAntiAlias;
        fHinting = SkFontHinting::kSlight;
        fSubpixel = true;
    }

    SkFont::Edging fEdging;
    SkFontHinting fHinting;
    bool fSubpixel;
};


}  // namespace textlayout
}  // namespace skia

#endif  // Metrics_DEFINED
