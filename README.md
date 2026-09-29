# Real-Time Camera ISP

A C++11-based camera image-processing project that simulates a simplified **Image Signal Processing (ISP) pipeline**. The project demonstrates camera-frame handling, Bayer RGGB representation, demosaicing, frame buffering, image statistics, and processing-time measurement.

The project is built from scratch using **C++**, without OpenCV or other external image-processing libraries.

## 📸 Project Demo

> Add your screenshots here after uploading them to the repository.

### Camera ISP GUI

<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/1ca00e44-1bba-406f-ba54-6bcbe9d478eb" />


### Input Image

<img width="779" height="487" alt="image" src="https://github.com/user-attachments/assets/6f3d6660-a6e6-417f-a875-5e3a451dfa83" />


### Demosaiced Output

<img width="790" height="489" alt="image" src="https://github.com/user-attachments/assets/faf6a28f-51fe-47a2-920b-7d77508f9274" />


---

## 🚀 Project Overview

A simplified camera pipeline can be represented as:

```text
Camera Frame
     ↓
Bayer RGGB Data
     ↓
Demosaicing
     ↓
RGB Image
     ↓
Display / Output
```

In a real camera system, the image sensor captures color-filtered pixel information rather than a complete RGB value for every pixel.

This project simulates that concept using PPM images and implements the processing pipeline in C++.

---

## ✨ Features

* P3 ASCII PPM image loading
* Camera frame abstraction
* Frame numbering
* Frame buffering
* Bayer RGGB pattern simulation
* Demosaicing
* RGB image processing
* RGB channel statistics
* Processing-time measurement
* Windows GUI
* Image loading through a Windows file dialog
* Processed image saving
* Multiple sample input images

---

## 🛠️ Technologies

* **C++11**
* **MinGW GCC 6.3.0**
* **Windows 11**
* **Visual Studio Code**
* **Win32 API**
* **GDI**
* **P3 PPM image format**

No OpenCV, CMake, Qt, CUDA, or other external image-processing libraries are required.

---

## 📁 Project Structure

```text
realtime-camera-isp/
│
├── include/
│   ├── image.h
│   ├── camera_frame.h
│   ├── frame_source.h
│   ├── frame_buffer.h
│   ├── demosaicing.h
│   └── timer.h
│
├── src/
│   ├── image.cpp
│   ├── frame_source.cpp
│   ├── frame_buffer.cpp
│   ├── demosaicing.cpp
│   └── timer.cpp
│
├── gui/
│   ├── gui.cpp
│   └── gui.h
│
├── tools/
│   └── make_test_image.cpp
│
├── data/
│   ├── input/
│   │   ├── image1.ppm
│   │   ├── image2.ppm
│   │   ├── image3.ppm
│   │   ├── image4.ppm
│   │   ├── image5.ppm
│   │   └── input.ppm
│   │
│   └── output/
│
├── main.cpp
└── README.md
```

---

## 🔄 Processing Pipeline

```text<img width="1024" height="559" alt="image" src="https://github.com/user-attachments/assets/db3ecc3b-92b7-473c-a568-0eef4aa524a4" />

```

---

## 🎨 Bayer RGGB Pattern

A simplified Bayer RGGB pattern used by the project is:

```text
R G R G
G B G B
R G R G
G B G B
```

Where:

* `R` = Red
* `G` = Green
* `B` = Blue

A camera sensor measures only one color component at each pixel.

Therefore, the missing color components need to be reconstructed to obtain a complete RGB image.

---

## 🧩 Demosaicing

The project implements a simplified demosaicing stage that converts Bayer-pattern information into an RGB representation.

Conceptually:

```text
Bayer Data
    ↓
Demosaicing
    ↓
RGB Image
```

The implementation is written manually in C++ to make the underlying pixel-processing operations easier to understand.

---

## 🧱 Frame Buffer

The project includes a frame buffer with a fixed capacity.

The simplified flow is:

```text
Camera / Input
      ↓
 Frame Source
      ↓
 Frame Buffer
      ↓
 Processing
```

If the buffer reaches its capacity and another frame arrives, the oldest frame can be removed to make space for the new frame.

This models a basic concept found in real-time camera pipelines.

---

## 📊 Image Statistics

The project calculates basic RGB statistics from the image, including:

* Mean red value
* Mean green value
* Mean blue value

These statistics provide a simple way to inspect the processed image data.

---

## ⏱️ Performance Measurement

The project uses C++ timing facilities to measure processing time.

The measured time depends on:

* Image resolution
* CPU
* Compiler
* Operating system
* System load

No fixed or fabricated performance numbers are used.

---

## 🖥️ GUI

The GUI is implemented using the Windows:

* **Win32 API**
* **GDI**

The interface provides functionality for:

* Loading an image
* Displaying image information
* Running the image-processing pipeline
* Viewing the processed result
* Saving the output

The GUI is intentionally kept simple so that the underlying image-processing pipeline remains the main focus.

---

## ▶️ Build and Run

### Requirements

* Windows 11
* MinGW GCC 6.3.0
* C++11
* Visual Studio Code

Check your compiler:

```cmd
g++ --version
```

Expected compiler family:

```text
g++ (MinGW.org GCC-6.3.0-1) 6.3.0
```

### Clone the repository

```cmd
git clone https://github.com/Nageshkumar01/realtime-camera-isp.git
```

Enter the project:

```cmd
cd realtime-camera-isp
```

### Compile

```cmd
g++ -std=c++11 -Iinclude main.cpp src\image.cpp src\frame_source.cpp src\frame_buffer.cpp src\demosaicing.cpp src\timer.cpp gui\gui.cpp -o realtime-camera-isp.exe -lgdi32 -luser32 -lcomdlg32
```

### Run

```cmd
realtime-camera-isp.exe
```

The Windows GUI will open.

---

## 🖼️ Input Images

Sample P3 PPM images are included in:

```text
data/input/
```

Examples:

```text
image1.ppm
image2.ppm
image3.ppm
image4.ppm
image5.ppm
input.ppm
```

The current implementation uses the **P3 ASCII PPM** format to keep image loading simple and dependency-free.

JPEG and PNG are not currently supported.

---

## 🎯 Why I Built This

I built this project to understand the fundamentals of camera image-processing pipelines using C++.

The project helped me work with:

* C++ classes
* Pixel-level processing
* Image representation
* Bayer patterns
* Demosaicing
* Frame buffering
* File I/O
* Performance measurement
* Windows GUI programming

Instead of relying on a high-level image-processing library, the core operations are implemented directly in C++.



## ⚠️ Limitations

Current limitations:

* Supports P3 ASCII PPM images only
* No JPEG/PNG support
* No physical camera input
* Bayer data is simulated
* Simplified demosaicing implementation
* No GPU acceleration
* No DSP acceleration
* No ARM NEON optimization
* Windows-specific GUI

---

## 🔮 Future Improvements

Possible future improvements include:

* Real camera input
* RAW Bayer image support
* More advanced demosaicing
* Bilinear demosaicing
* Edge-aware demosaicing
* White balance
* Color correction
* Gamma correction
* Noise reduction
* Image sharpening
* Histogram analysis
* Multithreaded processing
* SIMD/NEON optimization
* Linux support
* Embedded ARM implementation

---

## 📚 What I Learned

Through this project, I gained practical experience with:

* C++11
* Object-oriented programming
* `std::vector`
* File handling
* Pixel-level image processing
* Bayer RGGB patterns
* Demosaicing
* Frame buffering
* Performance measurement
* Win32 API
* GDI
* Modular C++ project organization

---

## 👨‍💻 Author

**Nagesh Kumar**

B.Tech Computer Science and Engineering

GitHub:
[https://github.com/Nageshkumar01](https://github.com/Nageshkumar01)

---

## 📌 Project Status

**Completed — portfolio/learning project**

The project currently includes the basic camera-processing pipeline, Bayer simulation, demosaicing, frame buffering, performance measurement, sample images, and Windows GUI visualization.
