/*
 * Copyright 2024 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "include/private/SkAssert.h"
#include "modules/skunicode/src/SkBidiFactory_icu_subset.h"

#include <unicode/uchar.h>
#include <unicode/umachine.h>

const char* SkBidiSubsetFactory::errorName(UErrorCode status) const {
    return u_errorName(status);
}

SkBidiFactory::BidiCloseCallback SkBidiSubsetFactory::bidi_close_callback() const {
    return ubidi_close;
}

UBiDiDirection SkBidiSubsetFactory::bidi_getDirection(const UBiDi* bidi) const {
    return ubidi_getDirection(bidi);
}

SkBidiIterator::Position SkBidiSubsetFactory::bidi_getLength(const UBiDi* bidi) const {
    return ubidi_getLength(bidi);
}

SkBidiIterator::Level SkBidiSubsetFactory::bidi_getLevelAt(const UBiDi* bidi, int pos) const {
    return ubidi_getLevelAt(bidi, pos);
}

UBiDi* SkBidiSubsetFactory::bidi_openSized(int32_t maxLength,
                                           int32_t maxRunCount,
                                           UErrorCode* pErrorCode) const {
    return ubidi_openSized(maxLength, maxRunCount, pErrorCode);
}

void SkBidiSubsetFactory::bidi_setPara(UBiDi* bidi,
                                       const UChar* text,
                                       int32_t length,
                                       UBiDiLevel paraLevel,
                                       UBiDiLevel* embeddingLevels,
                                       UErrorCode* status) const {
    return ubidi_setPara(bidi, text, length, paraLevel, embeddingLevels, status);
}

void SkBidiSubsetFactory::bidi_reorderVisual(const SkUnicode::BidiLevel runLevels[],
                                             int levelsCount,
                                             int32_t logicalFromVisual[]) const {
    if (levelsCount == 0) {
        // To avoid an assert in unicode
        return;
    }
    SkASSERT(runLevels != nullptr);
    ubidi_reorderVisual(runLevels, levelsCount, logicalFromVisual);
}

// SkUnicode::CharDirection reuses ICU's UCharDirection numbering
static_assert(static_cast<int>(SkUnicode::CharDirection::kLeftToRight) == U_LEFT_TO_RIGHT);
static_assert(static_cast<int>(SkUnicode::CharDirection::kRightToLeftArabic) == U_RIGHT_TO_LEFT_ARABIC);
static_assert(static_cast<int>(SkUnicode::CharDirection::kBoundaryNeutral) == U_BOUNDARY_NEUTRAL);
static_assert(static_cast<int>(SkUnicode::CharDirection::kPopDirectionalIsolate) ==
              U_POP_DIRECTIONAL_ISOLATE);

SkUnicode::CharDirection SkBidiSubsetFactory::charDirection(SkUnichar unichar) const {
    return static_cast<SkUnicode::CharDirection>(u_charDirection(unichar));
}
