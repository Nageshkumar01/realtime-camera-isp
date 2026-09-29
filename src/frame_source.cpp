#include "frame_source.h"

namespace isp {

FrameSource::FrameSource(const std::string& imagePath)
    : path_(imagePath), image_(Image::loadPPM(imagePath)), nextFrameNumber_(0) {}

CameraFrame FrameSource::nextFrame() {
    // CameraFrame's constructor takes the Image by value, so each frame gets
    // its own copy of the pixels. Changing one frame cannot change another.
    return CameraFrame(image_, nextFrameNumber_++);
}

}  // namespace isp