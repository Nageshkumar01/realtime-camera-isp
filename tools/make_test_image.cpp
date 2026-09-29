// Creates a test image as an ASCII PPM (P3) file.
// Usage: make_test_image [output.ppm] [width] [height]
// Defaults: data/input/input.ppm 256 192   (minimum size 8 x 8)

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include "image.h"

int main(int argc, char* argv[]) {
    const std::string outputPath = (argc > 1) ? argv[1] : "data/input/input.ppm";
    const int width = (argc > 2) ? std::atoi(argv[2]) : 256;
    const int height = (argc > 3) ? std::atoi(argv[3]) : 192;

    if (width < 8 || height < 8) {
        std::cerr << "Width and height must be at least 8.\n";
        return 1;
    }

    try {
        isp::Image image(width, height, isp::PixelFormat::RGB8);

        const int centerX = width / 2;
        const int centerY = height / 2;
        const int radius = std::min(width, height) / 4;
        const int stripeStart = height * 4 / 5;               // bottom 20 %
        const int bandWidth = std::max(1, width / 16);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int r, g, b;
                if (y >= stripeStart) {
                    // alternating pure red / pure blue vertical bands
                    const bool red = ((x / bandWidth) % 2 == 0);
                    r = red ? 255 : 0;
                    g = 0;
                    b = red ? 0 : 255;
                } else if ((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY)
                           <= radius * radius) {
                    r = 255; g = 220; b = 0;                  // yellow circle
                } else {
                    r = 255 * x / (width - 1);                // smooth gradient
                    g = 255 * y / (height - 1);
                    b = 255 - r;
                }
                image.at(x, y, 0) = static_cast<unsigned char>(r);
                image.at(x, y, 1) = static_cast<unsigned char>(g);
                image.at(x, y, 2) = static_cast<unsigned char>(b);
            }
        }

        image.savePPM(outputPath);
        std::cout << "Wrote " << width << " x " << height << " image to "
                  << outputPath << "\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}