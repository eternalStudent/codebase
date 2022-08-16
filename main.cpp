#include "basic/basic.cpp"
#include "graphics/graphics.cpp"
#include "ui/ui.cpp"

int main() {
	Arena scratch = CreateArena(1024*1024*1024);
	Arena persist = CreateArena(1024*1024);

	UIInit(&persist, &scratch);
	UICreateWindow("OpenGL Window", {512, 512}, RGBA_DARKGREY);

	Font* font = (Font*)ArenaAlloc(&persist, sizeof(Font));
	*font = LoadFont(&scratch, "..\\AzeretMono-Regular.ttf", 24);

	UIElement* e1 = UICreateElement(NULL);
	e1->background = RGBA_ORANGE;
	e1->pos = {300, 200};
	e1->dim = {100, 100};
	e1->flags = UI_MOVABLE | UI_RESIZABLE;

	UIElement* e2 = UICreateElement(NULL);
	e2->background = RGBA_BLUE;
	e2->pos = {300, 300};
	e2->dim = {100, 100};
	e2->flags = UI_MOVABLE | UI_RESIZABLE;

	UIElement* e3 = UICreateElement(NULL);
	e3->background = RGBA_GREEN;
	e3->pos = {200, 200};
	e3->dim = {100, 100};
	e3->flags = UI_MOVABLE | UI_RESIZABLE;

	while(!OSWindowDestroyed() && !OSIsKeyDown(KEY_ESC)) {
		ArenaFreeAll(&scratch);
		OSProcessWindowEvents();

		GfxClearScreen();

		UIUpdateActiveElement();
		UIRenderElements();

		GfxSwapBuffers();
	}
}

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
void __stdcall WinMainCRTStartup() {
    ExitProcess(main());
}
#endif