#pragma once

#include <chrono>
#include <utility>

#include "image.h"

namespace isp {

// One "frame" coming out of the (simulated) camera.
// width / height / format repeat information that is also inside `image`;
// they are kept here so code that only cares about frame metadata does not
// need to look inside the image.
struct CameraFrame {
    Image image;
    int width;
    int height;
    PixelFormat format;
    unsigned long long frameNumber;
    std::chrono::steady_clock::time_point timestamp;

    CameraFrame()
        : width(0), height(0), format(PixelFormat::RGB8), frameNumber(0) {}

    // Members are initialized in the order they are declared, so `image`
    // is ready before width / height / format read from it.
    CameraFrame(Image img, unsigned long long number)
        : image(std::move(img)),
          width(image.width()),
          height(image.height()),
          format(image.format()),
          frameNumber(number),
          timestamp(std::chrono::steady_clock::now()) {}
};

}  // namespace isp