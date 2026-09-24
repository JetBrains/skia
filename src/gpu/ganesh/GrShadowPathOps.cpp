/*
 * Copyright 2026 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/gpu/ganesh/GrShadowPathOps.h"

#include "include/core/SkPath.h"
#include "src/gpu/ganesh/GrStyle.h"
#include "src/gpu/ganesh/geometry/GrPathUtils.h"
#include "src/gpu/ganesh/geometry/GrStyledShape.h"
#include "src/utils/SkShadowPathOps.h"

namespace skgpu::ganesh {
namespace {

class GrShadowPathOps final : public SkShadowPathOps {
public:
    int keyBytes(const SkPath& path) const override {
        GrStyledShape shape(path, GrStyle::SimpleFill());
        return shape.hasUnstyledKey() ? shape.unstyledKeySize() * sizeof(uint32_t) : -1;
    }

    void writeKey(const SkPath& path, void* key) const override {
        GrStyledShape(path, GrStyle::SimpleFill()).writeUnstyledKey(
                reinterpret_cast<uint32_t*>(key));
    }

    uint32_t quadraticPointCount(const SkPoint points[3], SkScalar tolerance) const override {
        return GrPathUtils::quadraticPointCount(points, tolerance);
    }

    uint32_t generateQuadraticPoints(const SkPoint points[3],
                                     SkScalar toleranceSquared,
                                     SkPoint** output,
                                     uint32_t pointsLeft) const override {
        return GrPathUtils::generateQuadraticPoints(points[0], points[1], points[2],
                                                    toleranceSquared, output, pointsLeft);
    }

    uint32_t cubicPointCount(const SkPoint points[4], SkScalar tolerance) const override {
        return GrPathUtils::cubicPointCount(points, tolerance);
    }

    uint32_t generateCubicPoints(const SkPoint points[4],
                                 SkScalar toleranceSquared,
                                 SkPoint** output,
                                 uint32_t pointsLeft) const override {
        return GrPathUtils::generateCubicPoints(points[0], points[1], points[2], points[3],
                                                toleranceSquared, output, pointsLeft);
    }
};

}  // namespace

const SkShadowPathOps* ShadowPathOps() {
    static const GrShadowPathOps ops;
    return &ops;
}

}  // namespace skgpu::ganesh
