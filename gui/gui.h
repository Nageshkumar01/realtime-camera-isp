#pragma once

namespace isp {

// Opens the "Real-Time Camera ISP" window and runs until it is closed.
// Returns the program's exit code. Deliberately no Windows types here, so
// main.cpp does not need to include <windows.h>.
int runGui();

}  // namespace isp