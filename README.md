This is the reusable portions of the code I write for my own projects in C-style C++, what I start with when I start a new project.

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
  - Memory
* Bitmap + PNG Decoding (slightly modified version of stb_image)
* Graphics layer (D3D11 and OpenGL)
* Bare bones UI
  - Rounded coreners AABB, shadows, curves
  - Text rendering + icons
  - Some important widgets, like: UISlider, UIList, UIColorPicker, UIMenu

Everything in here should be treated as WIP, specifically I haven't updated Linux implementaion of the Platfrom Layer in quite a while, and only slightly ever touched Android.
