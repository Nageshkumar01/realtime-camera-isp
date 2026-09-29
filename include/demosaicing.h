#pragma once

#include "image.h"

namespace isp {

// Pretends a sensor captured `rgb`: keeps ONE color value per pixel in the
// RGGB pattern and throws the other two away.
// Input must be RGB8. Output is BAYER_RGGB (1 channel).
Image simulateBayerRGGB(const Image& rgb);

// Makes a colored picture of a Bayer image for display only:
// every pixel keeps its own color (R, G or B) and the other two channels
// are zero, so you can SEE the red / green / blue mosaic.
// Input must be BAYER_RGGB. Output is RGB8.
Image makeBayerColorPreview(const Image& bayer);

// Bilinear demosaicing: rebuilds a full RGB image from a Bayer RGGB image.
// Input must be BAYER_RGGB and at least 2 x 2 pixels. Output is RGB8.
Image demosaicBilinearRGGB(const Image& bayer);

// Everything the GUI (or the command line) needs from one run.
struct DemosaicRun {
    Image bayer;                    // 1-channel mosaic
    Image bayerPreview;             // colored picture of the mosaic
    Image rgb;                      // demosaiced result
    double demosaicMicroseconds;    // time spent in demosaicBilinearRGGB only

    DemosaicRun() : demosaicMicroseconds(0.0) {}
};

// input (RGB8) -> Bayer simulation -> bilinear demosaicing, with timing.
// The GUI calls only this function, so it knows nothing about the algorithm.
DemosaicRun runDemosaicPipeline(const Image& input);

}  // namespace isp