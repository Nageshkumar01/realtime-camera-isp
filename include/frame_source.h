#pragma once

#include <string>

#include "camera_frame.h"
#include "image.h"

namespace isp {

// A pretend camera that "captures" frames from a PPM (P3) file.
// The file is loaded once. Every nextFrame() call returns a copy of that
// picture with a new frame number, like a camera looking at a still scene.
class FrameSource {
public:
    // Throws std::runtime_error if the file cannot be loaded.
    explicit FrameSource(const std::string& imagePath);

    CameraFrame nextFrame();

    const std::string& path() const { return path_; }
    unsigned long long framesProduced() const { return nextFrameNumber_; }

private:
    std::string path_;
    Image image_;
    unsigned long long nextFrameNumber_;
};

}  // namespace isp