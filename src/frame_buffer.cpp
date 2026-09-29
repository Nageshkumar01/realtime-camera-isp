#include "frame_buffer.h"

#include <stdexcept>
#include <utility>

namespace isp {

FrameBuffer::FrameBuffer(std::size_t capacity)
    : capacity_(capacity), droppedFrames_(0) {
    if (capacity == 0) {
        throw std::invalid_argument("FrameBuffer: capacity must be at least 1");
    }
}

bool FrameBuffer::push(CameraFrame frame) {
    bool droppedOldFrame = false;

    if (full()) {
        frames_.pop_front();  // throw away the oldest frame
        ++droppedFrames_;
        droppedOldFrame = true;
    }

    frames_.push_back(std::move(frame));
    return droppedOldFrame;
}

bool FrameBuffer::pop(CameraFrame& out) {
    if (frames_.empty()) {
        return false;
    }
    out = std::move(frames_.front());
    frames_.pop_front();
    return true;
}

}  // namespace isp