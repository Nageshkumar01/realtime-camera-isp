// Win32 GUI for the Real-Time Camera ISP demo.
// This file only handles windows, buttons and drawing. All image processing
// happens in demosaicing.cpp (runDemosaicPipeline).

#ifndef NOMINMAX
#define NOMINMAX  // stops windows.h from defining min()/max() macros
#endif
#include <windows.h>
#include <commdlg.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "gui.h"

#include "camera_frame.h"
#include "demosaicing.h"
#include "frame_buffer.h"
#include "frame_source.h"
#include "image.h"

namespace isp {

namespace {

// ---------------------------------------------------------------------------
// Layout numbers (all in pixels, inside the window's client area)
// ---------------------------------------------------------------------------
const int kClientWidth  = 1040;
const int kClientHeight = 620;
const int kMargin       = 40;
const int kPanelSize    = 300;   // each image panel is 300 x 300
const int kPanelGap     = 30;    // space between panels (arrows go here)
const int kPanelTop     = 95;
const int kButtonWidth  = 150;
const int kButtonHeight = 36;
const int kButtonGap    = 20;
const int kButtonTop    = 565;

const char kWindowClassName[] = "RealTimeCameraIspWindow";
const char kOutputPath[]      = "data/output/demosaiced.ppm";

// Every button gets a number (ID). Windows sends it back in WM_COMMAND.
enum ButtonId {
    kIdLoad = 101,
    kIdRun  = 102,
    kIdSave = 103,
    kIdExit = 104
};

// ---------------------------------------------------------------------------
// BitmapHandle: owns one HBITMAP and deletes it automatically (RAII).
// Windows bitmaps are "GDI objects" that must be freed with DeleteObject,
// otherwise the program leaks them. This class makes that automatic.
// ---------------------------------------------------------------------------
class BitmapHandle {
public:
    BitmapHandle() : handle_(NULL) {}
    ~BitmapHandle() { reset(NULL); }

    // Frees the old bitmap (if any) and takes ownership of the new one.
    void reset(HBITMAP newHandle) {
        if (handle_ != NULL) {
            DeleteObject(handle_);
        }
        handle_ = newHandle;
    }
    HBITMAP get() const { return handle_; }

private:
    BitmapHandle(const BitmapHandle&) = delete;             // no copying:
    BitmapHandle& operator=(const BitmapHandle&) = delete;  // one owner only
    HBITMAP handle_;
};

// ---------------------------------------------------------------------------
// Image -> Windows bitmap
// A Windows "DIB" bitmap stores pixels as B, G, R (not R, G, B), and every
// row must be padded to a multiple of 4 bytes. We create the bitmap, then
// copy our pixels in with that layout.
// ---------------------------------------------------------------------------
HBITMAP createBitmapFromImage(const Image& image) {
    BITMAPINFO info;
    ZeroMemory(&info, sizeof(info));
    info.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth       = image.width();
    info.bmiHeader.biHeight      = -image.height();  // negative = top row first
    info.bmiHeader.biPlanes      = 1;
    info.bmiHeader.biBitCount    = 24;               // 3 bytes per pixel
    info.bmiHeader.biCompression = BI_RGB;           // uncompressed

    void* bits = NULL;  // Windows gives us a pointer to the pixel memory
    HBITMAP bitmap = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, &bits, NULL, 0);
    if (bitmap == NULL || bits == NULL) {
        throw std::runtime_error("CreateDIBSection failed (could not create bitmap)");
    }

    const int stride = ((image.width() * 3 + 3) / 4) * 4;  // bytes per row
    unsigned char* destination = static_cast<unsigned char*>(bits);

    for (int y = 0; y < image.height(); ++y) {
        unsigned char* row = destination + static_cast<std::size_t>(y) * stride;
        for (int x = 0; x < image.width(); ++x) {
            unsigned char r, g, b;
            if (image.channels() == 3) {
                r = image.at(x, y, 0);
                g = image.at(x, y, 1);
                b = image.at(x, y, 2);
            } else {
                r = g = b = image.at(x, y, 0);  // gray: same value 3 times
            }
            row[x * 3 + 0] = b;
            row[x * 3 + 1] = g;
            row[x * 3 + 2] = r;
        }
    }
    return bitmap;
}

// ---------------------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------------------

// Draws `bitmap` centered inside `panel`, keeping the aspect ratio.
// Small images are enlarged by a whole number (e.g. 8x8 -> 296x296) with no
// smoothing, so you can see the individual pixels. Big images are shrunk.
void drawBitmapFit(HDC hdc, HBITMAP bitmap, int srcW, int srcH, const RECT& panel) {
    const int panelW = panel.right - panel.left;
    const int panelH = panel.bottom - panel.top;

    double scale = std::min(static_cast<double>(panelW) / srcW,
                            static_cast<double>(panelH) / srcH);
    const bool enlarging = (scale >= 1.0);
    if (enlarging) {
        scale = std::floor(scale);  // whole-number zoom = square, sharp pixels
    }

    const int dstW = std::max(1, static_cast<int>(srcW * scale));
    const int dstH = std::max(1, static_cast<int>(srcH * scale));
    const int dstX = panel.left + (panelW - dstW) / 2;
    const int dstY = panel.top + (panelH - dstH) / 2;

    // A bitmap can only be drawn from a "memory DC": a hidden drawing surface
    // that holds the bitmap. Select it in, copy from it, then clean up.
    HDC memoryDc = CreateCompatibleDC(hdc);
    HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);

    if (enlarging) {
        SetStretchBltMode(hdc, COLORONCOLOR);  // no smoothing: crisp pixels
    } else {
        SetStretchBltMode(hdc, HALFTONE);      // smoother when shrinking
        SetBrushOrgEx(hdc, 0, 0, NULL);        // required after HALFTONE
    }
    StretchBlt(hdc, dstX, dstY, dstW, dstH, memoryDc, 0, 0, srcW, srcH, SRCCOPY);

    SelectObject(memoryDc, oldBitmap);
    DeleteDC(memoryDc);
}

void drawText(HDC hdc, int x, int y, const std::string& text) {
    TextOutA(hdc, x, y, text.c_str(), static_cast<int>(text.size()));
}

// Draws one image panel: title above, gray box, the picture (or a hint), border.
void drawImagePanel(HDC hdc, const RECT& panel, const char* title,
                    const Image& image, const BitmapHandle& bitmap) {
    TextOutA(hdc, panel.left, panel.top - 24, title, static_cast<int>(std::strlen(title)));
    FillRect(hdc, &panel, static_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH)));

    if (image.empty() || bitmap.get() == NULL) {
        RECT textRect = panel;
        DrawTextA(hdc, "(no image yet)", -1, &textRect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else {
        drawBitmapFit(hdc, bitmap.get(), image.width(), image.height(), panel);
    }
    FrameRect(hdc, &panel, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
}

void drawArrow(HDC hdc, int x1, int x2, int y) {
    MoveToEx(hdc, x1, y, NULL);
    LineTo(hdc, x2, y);
    MoveToEx(hdc, x2, y, NULL);
    LineTo(hdc, x2 - 7, y - 5);
    MoveToEx(hdc, x2, y, NULL);
    LineTo(hdc, x2 - 7, y + 5);
}

RECT panelRect(int index) {  // index 0, 1, 2 = left, middle, right panel
    RECT r;
    r.left   = kMargin + index * (kPanelSize + kPanelGap);
    r.top    = kPanelTop;
    r.right  = r.left + kPanelSize;
    r.bottom = r.top + kPanelSize;
    return r;
}

// ---------------------------------------------------------------------------
// MainWindow: the one window of the application.
// It keeps the images and bitmaps, reacts to messages from Windows, and
// calls into the ISP code when a button is clicked.
// ---------------------------------------------------------------------------
class MainWindow {
public:
    MainWindow()
        : hInstance_(NULL), hwnd_(NULL), loadButton_(NULL), runButton_(NULL),
          saveButton_(NULL), exitButton_(NULL), titleFont_(NULL),
          processingMicroseconds_(0.0), hasResult_(false),
          status_("Click 'Load Image' to start.") {}

    ~MainWindow() {
        if (titleFont_ != NULL) {
            DeleteObject(titleFont_);
        }
    }

    // Registers the window class, creates the window and shows it.
    bool create(HINSTANCE hInstance);

private:
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void createControls();
    void onPaint();
    void onCommand(int buttonId);
    void onLoadImage();
    void onRunDemosaicing();
    void onSaveOutput();
    void updateButtons();
    void showError(const std::string& message);
    HWND createButton(const char* text, int id, int x);

    HINSTANCE hInstance_;
    HWND hwnd_;
    HWND loadButton_;
    HWND runButton_;
    HWND saveButton_;
    HWND exitButton_;
    HFONT titleFont_;

    // Data shown in the three panels
    Image input_;
    Image bayerPreview_;
    Image output_;
    BitmapHandle inputBitmap_;
    BitmapHandle bayerBitmap_;
    BitmapHandle outputBitmap_;

    double processingMicroseconds_;
    bool hasResult_;
    std::string status_;
};

bool MainWindow::create(HINSTANCE hInstance) {
    hInstance_ = hInstance;

    // 1) Describe a "window class": the shared behaviour of our window.
    WNDCLASSA windowClass;
    ZeroMemory(&windowClass, sizeof(windowClass));
    windowClass.lpfnWndProc   = &MainWindow::windowProc;  // who handles messages
    windowClass.hInstance     = hInstance;
    windowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);  // white
    windowClass.lpszClassName = kWindowClassName;
    if (!RegisterClassA(&windowClass)) {
        return false;
    }

    // 2) We want a 1040 x 620 usable area. AdjustWindowRect adds the space
    //    the title bar and borders need.
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                        WS_MINIMIZEBOX | WS_CLIPCHILDREN;  // fixed size window
    RECT size = {0, 0, kClientWidth, kClientHeight};
    AdjustWindowRect(&size, style, FALSE);

    // 3) Create the window. The last argument (`this`) is delivered to
    //    windowProc during creation so it can find this C++ object.
    HWND hwnd = CreateWindowA(kWindowClassName, "Real-Time Camera ISP", style,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              size.right - size.left, size.bottom - size.top,
                              NULL, NULL, hInstance, this);
    if (hwnd == NULL) {
        return false;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return true;
}

// Windows calls this plain function for every message. It must be static
// because Windows knows nothing about our class. We use a small trick to get
// back to the MainWindow object: store its address inside the window itself.
LRESULT CALLBACK MainWindow::windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = NULL;

    if (message == WM_NCCREATE) {
        // First message of a new window: pick up the pointer passed to
        // CreateWindowA and remember it.
        CREATESTRUCTA* creation = reinterpret_cast<CREATESTRUCTA*>(lParam);
        self = static_cast<MainWindow*>(creation->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (self != NULL) {
        return self->handleMessage(hwnd, message, wParam, lParam);
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            createControls();
            return 0;

        case WM_PAINT:
            onPaint();
            return 0;

        case WM_COMMAND:  // a button was clicked
            if (HIWORD(wParam) == BN_CLICKED) {
                onCommand(LOWORD(wParam));
            }
            return 0;

        case WM_DESTROY:  // window is closing: end the message loop
            PostQuitMessage(0);
            return 0;
    }
    // Anything we don't handle gets Windows' default behaviour
    // (moving the window, minimizing, ...).
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

HWND MainWindow::createButton(const char* text, int id, int x) {
    // A button is just a child window of the predefined class "BUTTON".
    HWND button = CreateWindowA(
        "BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        x, kButtonTop, kButtonWidth, kButtonHeight,
        hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInstance_, NULL);
    SendMessage(button, WM_SETFONT,
                reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    return button;
}

void MainWindow::createControls() {
    titleFont_ = CreateFontA(26, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    const int rowWidth = 4 * kButtonWidth + 3 * kButtonGap;
    int x = (kClientWidth - rowWidth) / 2;  // center the row of buttons

    loadButton_ = createButton("Load Image", kIdLoad, x);
    x += kButtonWidth + kButtonGap;
    runButton_ = createButton("Run Demosaicing", kIdRun, x);
    x += kButtonWidth + kButtonGap;
    saveButton_ = createButton("Save Output", kIdSave, x);
    x += kButtonWidth + kButtonGap;
    exitButton_ = createButton("Exit", kIdExit, x);

    updateButtons();
}

// Grey out buttons that make no sense yet.
void MainWindow::updateButtons() {
    EnableWindow(runButton_, input_.empty() ? FALSE : TRUE);
    EnableWindow(saveButton_, hasResult_ ? TRUE : FALSE);
}

void MainWindow::showError(const std::string& message) {
    status_ = "Error: " + message;
    MessageBoxA(hwnd_, message.c_str(), "Real-Time Camera ISP", MB_OK | MB_ICONERROR);
    InvalidateRect(hwnd_, NULL, TRUE);
}

// Called whenever Windows says part of the window needs to be redrawn.
// We simply redraw everything from our current data.
void MainWindow::onPaint() {
    PAINTSTRUCT paint;
    HDC hdc = BeginPaint(hwnd_, &paint);
    SetBkMode(hdc, TRANSPARENT);  // text without a colored box behind it

    // Title
    HGDIOBJ originalFont = SelectObject(hdc, titleFont_);
    RECT titleRect = {0, 8, kClientWidth, 40};
    DrawTextA(hdc, "REAL-TIME CAMERA ISP", -1, &titleRect, DT_CENTER | DT_SINGLELINE);

    // Everything else uses the normal GUI font
    SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
    RECT captionRect = {0, 44, kClientWidth, 64};
    DrawTextA(hdc, "Input Image  ->  Bayer RGGB  ->  Bilinear Demosaicing  ->  RGB Output",
              -1, &captionRect, DT_CENTER | DT_SINGLELINE);

    // The three image panels and the arrows between them
    const RECT left = panelRect(0);
    const RECT middle = panelRect(1);
    const RECT right = panelRect(2);
    drawImagePanel(hdc, left, "1. INPUT IMAGE (RGB)", input_, inputBitmap_);
    drawImagePanel(hdc, middle, "2. BAYER RGGB (one color per pixel)", bayerPreview_, bayerBitmap_);
    drawImagePanel(hdc, right, "3. DEMOSAICED IMAGE (RGB)", output_, outputBitmap_);

    const int arrowY = kPanelTop + kPanelSize / 2;
    drawArrow(hdc, left.right + 4, middle.left - 4, arrowY);
    drawArrow(hdc, middle.right + 4, right.left - 4, arrowY);

    // Information box
    RECT infoBox = {kMargin, 415, kClientWidth - kMargin, 545};
    FrameRect(hdc, &infoBox, static_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));

    std::ostringstream resolution;
    resolution << "Resolution: ";
    if (input_.empty()) {
        resolution << "-";
    } else {
        resolution << input_.width() << " x " << input_.height();
    }

    std::ostringstream timeText;
    timeText << "Processing Time: ";
    if (hasResult_) {
        timeText << std::fixed << std::setprecision(2) << processingMicroseconds_
                 << " microseconds (demosaicing only)";
    } else {
        timeText << "-";
    }

    const int textX = kMargin + 12;
    drawText(hdc, textX, 424, "Bayer Pattern: RGGB");
    drawText(hdc, textX, 448, "Algorithm: Bilinear Demosaicing");
    drawText(hdc, textX, 472, resolution.str());
    drawText(hdc, textX, 496, timeText.str());
    drawText(hdc, textX, 520, "Status: " + status_);

    SelectObject(hdc, originalFont);
    EndPaint(hwnd_, &paint);
}

void MainWindow::onCommand(int buttonId) {
    switch (buttonId) {
        case kIdLoad: onLoadImage();      break;
        case kIdRun:  onRunDemosaicing(); break;
        case kIdSave: onSaveOutput();     break;
        case kIdExit: DestroyWindow(hwnd_); break;  // triggers WM_DESTROY
    }
}

void MainWindow::onLoadImage() {
    // Standard Windows "Open file" dialog.
    char fileName[MAX_PATH] = "";
    OPENFILENAMEA dialog;
    ZeroMemory(&dialog, sizeof(dialog));
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd_;
    dialog.lpstrFilter = "PPM images (*.ppm)\0*.ppm\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = fileName;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrInitialDir = "data\\input";
    // NOCHANGEDIR: keep the program's working folder, otherwise the relative
    // path used by "Save Output" would break after browsing to another folder.
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameA(&dialog)) {
        return;  // user pressed Cancel
    }

    try {
        // Same path as Phase 1: FrameSource -> FrameBuffer -> processing.
        FrameSource source(fileName);
        FrameBuffer buffer(4);
        buffer.push(source.nextFrame());

        CameraFrame frame;
        if (!buffer.pop(frame)) {
            throw std::runtime_error("frame buffer unexpectedly empty");
        }

        input_ = frame.image;
        inputBitmap_.reset(createBitmapFromImage(input_));

        // Old results belong to the old image: clear them.
        bayerPreview_ = Image();
        output_ = Image();
        bayerBitmap_.reset(NULL);
        outputBitmap_.reset(NULL);
        hasResult_ = false;

        status_ = std::string("Loaded ") + fileName + ". Now click 'Run Demosaicing'.";
    } catch (const std::exception& error) {
        showError(error.what());
        return;
    }

    updateButtons();
    InvalidateRect(hwnd_, NULL, TRUE);  // ask Windows to repaint (WM_PAINT)
}

void MainWindow::onRunDemosaicing() {
    if (input_.empty()) {
        return;
    }

    try {
        // The only line where the GUI touches the ISP code.
        const DemosaicRun run = runDemosaicPipeline(input_);

        bayerPreview_ = run.bayerPreview;
        output_ = run.rgb;
        processingMicroseconds_ = run.demosaicMicroseconds;
        hasResult_ = true;

        bayerBitmap_.reset(createBitmapFromImage(bayerPreview_));
        outputBitmap_.reset(createBitmapFromImage(output_));

        status_ = "Demosaicing finished. You can save the result.";
    } catch (const std::exception& error) {
        showError(error.what());
        return;
    }

    updateButtons();
    InvalidateRect(hwnd_, NULL, TRUE);
}

void MainWindow::onSaveOutput() {
    if (!hasResult_) {
        return;
    }

    try {
        // Make sure data\output exists (fails harmlessly if it already does).
        CreateDirectoryA("data", NULL);
        CreateDirectoryA("data\\output", NULL);

        output_.savePPM(kOutputPath);
        status_ = std::string("Saved to ") + kOutputPath;
    } catch (const std::exception& error) {
        showError(error.what());
        return;
    }
    InvalidateRect(hwnd_, NULL, TRUE);
}

}  // namespace

int runGui() {
    HINSTANCE hInstance = GetModuleHandleA(NULL);

    MainWindow window;
    if (!window.create(hInstance)) {
        MessageBoxA(NULL, "Could not create the window.", "Real-Time Camera ISP",
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    // The message loop: the heart of every Windows program.
    // GetMessage waits until something happens (mouse click, key press,
    // "please repaint"...), TranslateMessage handles keyboard text, and
    // DispatchMessage hands the message to windowProc.
    // GetMessage returns 0 when PostQuitMessage was called, ending the loop.
    MSG message;
    while (GetMessageA(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return static_cast<int>(message.wParam);
}

}  // namespace isp