#include "SkikoTest.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPicture.h"
#include "include/core/SkPictureRecorder.h"
#include "include/core/SkRect.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSurface.h"

// A layer's bounds restrict what it draws, and Skiko's RenderNode expresses a node's
// clip-to-bounds that way. Recording those calls and replaying the picture therefore has to
// match issuing them straight to a canvas: no record optimization may drop the bounds along
// with the layer.
DEF_TEST_SKIKO(SaveLayerBoundsClipRecordedContent, reporter) {
    static constexpr int kSurfaceSize = 10;
    static constexpr SkScalar kLayerSize = 4;
    // Outside the layer, where dropping its bounds would let the draw through.
    static constexpr int kSampleX = 8, kSampleY = 8;

    // A single draw overflowing the layer: the SaveLayer-draw-Restore run the peephole
    // optimizations match on.
    auto draw = [](SkCanvas* canvas) {
        canvas->drawColor(SK_ColorWHITE);
        SkPaint layerPaint;
        layerPaint.setAlphaf(0.5f);
        canvas->saveLayer(SkRect::MakeWH(kLayerSize, kLayerSize), &layerPaint);
        canvas->drawRect(SkRect::MakeWH(kSurfaceSize, kSurfaceSize), SkPaint());
        canvas->restore();
    };

    const SkImageInfo info = SkImageInfo::MakeN32Premul(kSurfaceSize, kSurfaceSize);
    sk_sp<SkSurface> direct = SkSurfaces::Raster(info);
    sk_sp<SkSurface> replayed = SkSurfaces::Raster(info);

    draw(direct->getCanvas());

    SkPictureRecorder recorder;
    draw(recorder.beginRecording(kSurfaceSize, kSurfaceSize));
    replayed->getCanvas()->drawPicture(recorder.finishRecordingAsPicture());

    const SkImageInfo pixelInfo = SkImageInfo::MakeN32Premul(1, 1);
    SkPMColor fromDirect = 0, fromReplay = 0;
    direct->readPixels(pixelInfo, &fromDirect, sizeof(SkPMColor), kSampleX, kSampleY);
    replayed->readPixels(pixelInfo, &fromReplay, sizeof(SkPMColor), kSampleX, kSampleY);

    REPORTER_ASSERT(reporter, fromDirect == SkPreMultiplyColor(SK_ColorWHITE));
    REPORTER_ASSERT(reporter, fromDirect == fromReplay);
}
