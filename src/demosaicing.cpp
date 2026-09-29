#include "demosaicing.h"

#include <stdexcept>

#include "timer.h"

namespace isp {

namespace {

// A neighbour position relative to the current pixel.
struct Offset {
    int dx;
    int dy;
};

const Offset kCross[4]      = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};    // up, down, left, right
const Offset kDiagonal[4]   = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};  // four corners
const Offset kHorizontal[2] = {{-1, 0}, {1, 0}};                     // left, right
const Offset kVertical[2]   = {{0, -1}, {0, 1}};                     // up, down

// Average of the raw values at the given neighbour positions.
// Neighbours outside the image are skipped, so a pixel on the border is
// averaged from fewer values. We never "invent" a neighbour, which would
// give it the wrong color. (For images at least 2x2, at least one neighbour
// of every list always exists.)
// Time: O(count), i.e. constant.
int averageOfNeighbours(const Image& raw, int x, int y,
                        const Offset* offsets, int count) {
    int sum = 0;
    int used = 0;
    for (int i = 0; i < count; ++i) {
        const int nx = x + offsets[i].dx;
        const int ny = y + offsets[i].dy;
        if (nx < 0 || ny < 0 || nx >= raw.width() || ny >= raw.height()) {
            continue;
        }
        sum += raw.at(nx, ny);
        ++used;
    }
    return (sum + used / 2) / used;  // rounded average
}

unsigned char toByte(int value) {
    return static_cast<unsigned char>(value);
}

}  // namespace

Image simulateBayerRGGB(const Image& rgb) {
    if (rgb.format() != PixelFormat::RGB8 || rgb.empty()) {
        throw std::invalid_argument("simulateBayerRGGB: input must be a non-empty RGB8 image");
    }

    Image mosaic(rgb.width(), rgb.height(), PixelFormat::BAYER_RGGB);

    for (int y = 0; y < rgb.height(); ++y) {
        for (int x = 0; x < rgb.width(); ++x) {
            const bool evenRow = (y % 2 == 0);
            const bool evenCol = (x % 2 == 0);

            if (evenRow && evenCol) {
                mosaic.at(x, y) = rgb.at(x, y, 0);   // R sensor pixel
            } else if (!evenRow && !evenCol) {
                mosaic.at(x, y) = rgb.at(x, y, 2);   // B sensor pixel
            } else {
                mosaic.at(x, y) = rgb.at(x, y, 1);   // G sensor pixel
            }
        }
    }
    return mosaic;
}

Image makeBayerColorPreview(const Image& bayer) {
    if (bayer.format() != PixelFormat::BAYER_RGGB || bayer.empty()) {
        throw std::invalid_argument("makeBayerColorPreview: input must be a non-empty Bayer image");
    }

    Image preview(bayer.width(), bayer.height(), PixelFormat::RGB8);  // starts black

    for (int y = 0; y < bayer.height(); ++y) {
        for (int x = 0; x < bayer.width(); ++x) {
            const bool evenRow = (y % 2 == 0);
            const bool evenCol = (x % 2 == 0);

            int channel = 1;                       // green by default
            if (evenRow && evenCol) channel = 0;   // red
            if (!evenRow && !evenCol) channel = 2; // blue

            preview.at(x, y, channel) = bayer.at(x, y);
        }
    }
    return preview;
}

Image demosaicBilinearRGGB(const Image& bayer) {
    if (bayer.format() != PixelFormat::BAYER_RGGB || bayer.empty()) {
        throw std::invalid_argument("demosaicBilinearRGGB: input must be a non-empty Bayer image");
    }
    if (bayer.width() < 2 || bayer.height() < 2) {
        throw std::invalid_argument("demosaicBilinearRGGB: image must be at least 2 x 2");
    }

    Image rgb(bayer.width(), bayer.height(), PixelFormat::RGB8);

    for (int y = 0; y < bayer.height(); ++y) {
        for (int x = 0; x < bayer.width(); ++x) {
            const bool evenRow = (y % 2 == 0);
            const bool evenCol = (x % 2 == 0);
            const int measured = bayer.at(x, y);
            int r = 0, g = 0, b = 0;

            if (evenRow && evenCol) {
                // RED pixel. Up/down/left/right neighbours are green,
                // the four diagonal neighbours are blue.
                r = measured;
                g = averageOfNeighbours(bayer, x, y, kCross, 4);
                b = averageOfNeighbours(bayer, x, y, kDiagonal, 4);
            } else if (!evenRow && !evenCol) {
                // BLUE pixel. Cross neighbours are green, diagonals are red.
                b = measured;
                g = averageOfNeighbours(bayer, x, y, kCross, 4);
                r = averageOfNeighbours(bayer, x, y, kDiagonal, 4);
            } else if (evenRow) {
                // GREEN pixel on a red row (row: R G R G).
                // Left/right neighbours are red, up/down are blue.
                g = measured;
                r = averageOfNeighbours(bayer, x, y, kHorizontal, 2);
                b = averageOfNeighbours(bayer, x, y, kVertical, 2);
            } else {
                // GREEN pixel on a blue row (row: G B G B).
                // Left/right neighbours are blue, up/down are red.
                g = measured;
                b = averageOfNeighbours(bayer, x, y, kHorizontal, 2);
                r = averageOfNeighbours(bayer, x, y, kVertical, 2);
            }

            rgb.at(x, y, 0) = toByte(r);
            rgb.at(x, y, 1) = toByte(g);
            rgb.at(x, y, 2) = toByte(b);
        }
    }
    return rgb;
}

DemosaicRun runDemosaicPipeline(const Image& input) {
    DemosaicRun run;
    run.bayer = simulateBayerRGGB(input);
    run.bayerPreview = makeBayerColorPreview(run.bayer);

    Timer timer;  // only the demosaicing itself is timed
    run.rgb = demosaicBilinearRGGB(run.bayer);
    run.demosaicMicroseconds = timer.elapsedMicroseconds();

    return run;
}

}  // namespace isp