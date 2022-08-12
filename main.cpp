#define LOG(text)	write(1, text, sizeof(text)-1)

#include "basic/basic.cpp"
#include "graphics/graphics.cpp"
#include "ui/ui.cpp"

int main() {
	OsCreateWindow("OpenGL Window", 512, 512);

	Arena scratch = CreateArena(1024);
	GraphicsInit(&scratch);
	GraphicsSetColor(0x0000ff);

	while(true) {
		OsHandleWindowEvents();
		if (OsWindowDestroyed() || IsKeyDown(KEY_ESC)) break; 

		GraphicsClearScreen();

		if (!GraphicsSwapBuffers()) {
			LOG("Failed to swap OpenGL buffers!");
		}
	}
}

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
void __stdcall WinMainCRTStartup()
{
    ExitProcess(main());
}
#endif