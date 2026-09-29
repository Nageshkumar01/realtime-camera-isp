#include "image.h"

#include <cctype>
#include <fstream>
#include <ios>
#include <istream>
#include <stdexcept>
#include <string>

namespace isp {

namespace {

// Reads the next integer from a PPM file.
// PPM allows comments: '#' until the end of the line. We skip those and any
// whitespace first. Returns false if there is no integer left.
bool readInt(std::istream& in, int& value) {
    for (;;) {
        const int c = in.peek();
        if (c == std::char_traits<char>::eof()) {
            return false;
        }
        if (c == '#') {
            std::string skippedComment;
            std::getline(in, skippedComment);
        } else if (std::isspace(c)) {
            in.get();
        } else {
            break;
        }
    }
    in >> value;
    return !in.fail();
}

}  // namespace

Image::Image(int width, int height, PixelFormat format)
    : width_(width), height_(height), format_(format) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Image: width and height must be positive");
    }
    pixels_.assign(static_cast<std::size_t>(width) * height * channelCount(format), 0);
}

Image Image::loadPPM(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file) {
        throw std::runtime_error("Could not open file: " + path);
    }

    std::string magic;
    file >> magic;
    if (magic != "P3") {
        throw std::runtime_error("Not an ASCII PPM (P3) file: " + path);
    }

    int width = 0;
    int height = 0;
    int maxValue = 0;
    if (!readInt(file, width) || !readInt(file, height) || !readInt(file, maxValue)) {
        throw std::runtime_error("Invalid PPM header in: " + path);
    }
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384) {
        throw std::runtime_error("Unsupported image size in: " + path);
    }
    if (maxValue <= 0 || maxValue > 65535) {
        throw std::runtime_error("Invalid PPM maximum value in: " + path);
    }

    Image image(width, height, PixelFormat::RGB8);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int c = 0; c < 3; ++c) {
                int value = 0;
                if (!readInt(file, value)) {
                    throw std::runtime_error("PPM file ended too early: " + path);
                }
                if (value < 0) value = 0;
                if (value > maxValue) value = maxValue;
                // Scale to 0..255 in case the file uses another maximum.
                image.at(x, y, c) =
                    static_cast<unsigned char>((value * 255 + maxValue / 2) / maxValue);
            }
        }
    }
    return image;
}

void Image::savePPM(const std::string& path) const {
    if (empty()) {
        throw std::runtime_error("Image::savePPM: image is empty");
    }

    std::ofstream out(path.c_str());
    if (!out) {
        throw std::runtime_error("Could not open file for writing: " + path);
    }

    out << "P3\n# Written by Real-Time Camera ISP\n"
        << width_ << " " << height_ << "\n255\n";

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (x > 0) {
                out << "  ";
            }
            for (int k = 0; k < 3; ++k) {
                if (k > 0) {
                    out << ' ';
                }
                // Gray and Bayer images only have channel 0.
                const int channel = (channels() == 1) ? 0 : k;
                out << static_cast<int>(at(x, y, channel));
            }
        }
        out << "\n";
    }

    if (!out) {
        throw std::runtime_error("Error while writing: " + path);
    }
}

}  // namespace isp