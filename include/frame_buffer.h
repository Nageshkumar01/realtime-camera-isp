#pragma once

#include <cstddef>
#include <deque>

#include "camera_frame.h"

namespace isp {

// A small first-in-first-out queue of frames with a fixed capacity.
// If the queue is full when a new frame arrives, the OLDEST frame is dropped
// (a real camera also drops frames when processing is too slow).
// Not thread-safe: use it from one thread only.
class FrameBuffer {
public:
    // Throws std::invalid_argument if capacity is 0.
    explicit FrameBuffer(std::size_t capacity);

    // Adds a frame. Returns true if an old frame had to be dropped.
    bool push(CameraFrame frame);

    // Removes the oldest frame and stores it in `out`.
    // Returns false (and leaves `out` unchanged) if the buffer is empty.
    bool pop(CameraFrame& out);

    std::size_t size() const { return frames_.size(); }
    std::size_t capacity() const { return capacity_; }
    bool empty() const { return frames_.empty(); }
    bool full() const { return frames_.size() >= capacity_; }
    std::size_t droppedFrames() const { return droppedFrames_; }

private:
    std::size_t capacity_;
    std::deque<CameraFrame> frames_;
    std::size_t droppedFrames_;
};

}  // namespace isp