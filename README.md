This is the reusable portions of the code I write for my own projects in C-style C++, the code I start with when I start a new project.

It contains:
* Basic types, bit operations, math operations
* String operationn, encodings
* Memory allocation startegies (arena, free-list)
* LinkedList macros
* Platform layer
  - I/O
  - Window creation
  - Input handling
  - Audio
  - Time measurements
  - Virtual Memory
* Bitmap + PNG Decoding (slightly modified version of stb_image)
* Graphics layer (D3D11 and OpenGL)
* Bare bones UI
  - Rounded coreners AABB, shadows, curves
  - Text rendering + icons
  - Some important widgets, like: UISlider, UIList, UIColorPicker, UIMenu

Text Rendering is done with DirectWrite on Window, and FreeType on Linux.

Everything in here should be treated as WIP, specifically audio support for Linux is missing, and I only slightly ever touched Android.
