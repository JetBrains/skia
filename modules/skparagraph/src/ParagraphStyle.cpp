// Copyright 2019 Google LLC.

#include "modules/skparagraph/include/DartTypes.h"
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "src/base/SkUTF.h"
#include "src/core/SkStringUtils.h"

namespace skia {
namespace textlayout {

StrutStyle::StrutStyle() {
    fFontStyle = SkFontStyle::Normal();
    fFontSize = 14;
    fHeight = 1;
    fLeading = -1;
    fForceHeight = false;
    fHeightOverride = false;
    fTopRatio = -1.0f;
    fEnabled = false;
}

TextIndent::TextIndent() {
    fFirstLine = 0.0;
    fRestLine = 0.0;
}


bool ParagraphStyle::equals(const ParagraphStyle& rhs) const {
    return this->fStrutStyle == rhs.fStrutStyle &&
           this->fDefaultTextStyle == rhs.fDefaultTextStyle &&
           this->fTextAlign == rhs.fTextAlign &&
           this->fTextDirection == rhs.fTextDirection &&
           this->fLinesLimit == rhs.fLinesLimit &&
           this->fEllipsisUtf16 == rhs.fEllipsisUtf16 &&
           this->fEllipsis == rhs.fEllipsis &&
           this->fHeight == rhs.fHeight &&
           this->fTextHeightBehavior == rhs.fTextHeightBehavior &&
           this->fTextIndent == rhs.fTextIndent &&
           this->fFontRastrSettings == rhs.fFontRastrSettings &&
           this->fHintingIsOn == rhs.fHintingIsOn &&
           this->fReplaceTabCharacters == rhs.fReplaceTabCharacters &&
           this->fApplyRoundingHack == rhs.fApplyRoundingHack;
}

bool ParagraphStyle::equalsByLayout(const ParagraphStyle& rhs) const {
    return this->fStrutStyle == rhs.fStrutStyle &&
           this->fDefaultTextStyle.equalsByFonts(rhs.fDefaultTextStyle) &&
           this->fTextAlign == rhs.fTextAlign &&
           this->fTextDirection == rhs.fTextDirection &&
           this->fLinesLimit == rhs.fLinesLimit &&
           this->fEllipsisUtf16 == rhs.fEllipsisUtf16 &&
           this->fEllipsis == rhs.fEllipsis &&
           nearlyEqual(fHeight, rhs.fHeight) &&
           this->fTextHeightBehavior == rhs.fTextHeightBehavior &&
           this->fTextIndent == rhs.fTextIndent &&
           this->fFontRastrSettings == rhs.fFontRastrSettings &&
           this->fHintingIsOn == rhs.fHintingIsOn &&
           this->fReplaceTabCharacters == rhs.fReplaceTabCharacters &&
           this->fApplyRoundingHack == rhs.fApplyRoundingHack;
}

ParagraphStyle::ParagraphStyle() {
    fTextAlign = TextAlign::kStart;
    fTextDirection = TextDirection::kLtr;
    fLinesLimit = std::numeric_limits<size_t>::max();
    fHeight = 1;
    fTextHeightBehavior = TextHeightBehavior::kAll;
    fHintingIsOn = true;
    fReplaceTabCharacters = false;
}

TextAlign ParagraphStyle::effective_align() const {
    if (fTextAlign == TextAlign::kStart) {
        return (fTextDirection == TextDirection::kLtr) ? TextAlign::kLeft : TextAlign::kRight;
    } else if (fTextAlign == TextAlign::kEnd) {
        return (fTextDirection == TextDirection::kLtr) ? TextAlign::kRight : TextAlign::kLeft;
    } else {
        return fTextAlign;
    }
}
}  // namespace textlayout
}  // namespace skia
