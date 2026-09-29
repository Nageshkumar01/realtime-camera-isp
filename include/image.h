#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace isp {

// The pixel layouts this project understands.
//   GRAY8       : 1 value per pixel
//   RGB8        : 3 values per pixel, in the order R, G, B
//   BAYER_RGGB  : 1 value per pixel. Each pixel holds only ONE color sample:
//
//                   col 0  col 1  col 2  col 3
//             row 0:  R      G      R      G
//             row 1:  G      B      G      B
//             row 2:  R      G      R      G
//             row 3:  G      B      G      B
enum class PixelFormat { GRAY8, RGB8, BAYER_RGGB };

// How many values one pixel needs for a format.
inline int channelCount(PixelFormat format) {
    return (format == PixelFormat::RGB8) ? 3 : 1;
}

inline std::string toString(PixelFormat format) {
    switch (format) {
        case PixelFormat::GRAY8:      return "GRAY8";
        case PixelFormat::RGB8:       return "RGB8";
        case PixelFormat::BAYER_RGGB: return "BAYER_RGGB";
    }
    return "UNKNOWN";
}

// A simple 8-bit image. Pixels are stored in one std::vector, row after row.
// For RGB8 the values are interleaved: R G B R G B ...
// Copying an Image copies all pixels (deep copy), because std::vector does.
class Image {
public:
    // An empty image (0 x 0).
    Image() : width_(0), height_(0), format_(PixelFormat::RGB8) {}

    // Creates a width x height image filled with zeros (black).
    // Throws std::invalid_argument if width or height is not positive.
    Image(int width, int height, PixelFormat format);

    bool empty() const { return pixels_.empty(); }
    int width() const { return width_; }
    int height() const { return height_; }
    PixelFormat format() const { return format_; }
    int channels() const { return channelCount(format_); }

    // Access one value of pixel (x, y). `c` is the channel (0 for gray/Bayer;
    // 0 = R, 1 = G, 2 = B for RGB8). NOT bounds-checked, so callers must stay
    // inside 0 <= x < width, 0 <= y < height.
    unsigned char& at(int x, int y, int c = 0) {
        return pixels_[(static_cast<std::size_t>(y) * width_ + x) * channels() + c];
    }
    const unsigned char& at(int x, int y, int c = 0) const {
        return pixels_[(static_cast<std::size_t>(y) * width_ + x) * channels() + c];
    }

    // Reads an ASCII PPM ("P3") file into an RGB8 image.
    // Throws std::runtime_error if the file is missing or malformed.
    static Image loadPPM(const std::string& path);

    // Writes the image as an ASCII PPM ("P3") file.
    // Gray / Bayer images are written with R = G = B = value.
    // The folder must already exist. Throws std::runtime_error on failure.
    void savePPM(const std::string& path) const;

private:
    int width_;
    int height_;
    PixelFormat format_;
    std::vector<unsigned char> pixels_;
};

}  // namespace isp