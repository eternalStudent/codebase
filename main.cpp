#include "basic/basic.cpp"
#include "graphics/graphics.cpp"
#include "ui/ui.cpp"

int main() {
	Arena scratch = CreateArena(1024*1024);
	Arena persist = CreateArena(1024*1024);

	UIInit(&persist, &scratch);
	UICreateWindow("OpenGL Window", {512, 512}, RGBA_DARKGREY);
	UIElement* element1 = UICreateElement(NULL);
	element1->background = RGBA_BLUE;
	element1->pos = {200, 200};
	element1->dim = {64, 64};
	element1->flags = UI_MOVABLE;

	while(!OsWindowDestroyed() && !IsKeyDown(KEY_ESC)) {
		OsProcessWindowEvents();

		GfxClearScreen();

		UIUpdateElements();
		UIRenderElements();

		GfxSwapBuffers();
	}
}

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
void __stdcall WinMainCRTStartup()
{
    ExitProcess(main());
}
#endif