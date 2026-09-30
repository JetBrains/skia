/*
 * Copyright 2026 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkShadowPathOps_DEFINED
#define SkShadowPathOps_DEFINED

#include "include/core/SkScalar.h"

#include <cstdint>

class SkPath;
struct SkPoint;

// Backend extension points used by the shared shadow tessellator. A null provider retains the
// backend-independent fallback behavior.
class SkShadowPathOps {
public:
    virtual ~SkShadowPathOps() = default;

    virtual int keyBytes(const SkPath&) const = 0;
    virtual void writeKey(const SkPath&, void*) const = 0;

    virtual uint32_t quadraticPointCount(const SkPoint[3], SkScalar tolerance) const = 0;
    virtual uint32_t generateQuadraticPoints(const SkPoint[3],
                                             SkScalar toleranceSquared,
                                             SkPoint**,
                                             uint32_t pointsLeft) const = 0;

    virtual uint32_t cubicPointCount(const SkPoint[4], SkScalar tolerance) const = 0;
    virtual uint32_t generateCubicPoints(const SkPoint[4],
                                         SkScalar toleranceSquared,
                                         SkPoint**,
                                         uint32_t pointsLeft) const = 0;
};

#endif
