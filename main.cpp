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
	e1->pos = {12, 12};
	e1->dim = {200, 200};
	e1->flags = UI_HIDE_OVERFLOW;
	e1->borderWidth = 1;
	e1->borderColor = RGBA_WHITE;
	UIText* text1 = UICreateText(e1);
	text1->font = font;
	text1->color = RGBA_WHITE;
	text1->string = STR(R"STRING(Lorem ipsum dolor sit amet, 
consectetur adipiscing elit, 
sed do eiusmod tempor incididunt 
ut labore et dolore magna aliqua. 
Ut enim ad minim veniam, quis nostrud 
exercitation ullamco laboris nisi ut 
aliquip ex ea commodo consequat.
Duis aute irure dolor in reprehenderit
in voluptate velit esse cillum dolore
eu fugiat nulla pariatur. Excepteur
sint occaecat cupidatat non proident, 
sunt in culpa qui officia deserunt
mollit anim id est laborum.)STRING");

	UIElement* debug1 = UICreateElement(NULL);
	debug1->pos = {12, 300};
	debug1->flags = UI_MOVABLE;
	debug1->background = 0x33ffffff;
	UIText* debugText1 = UICreateText(debug1);
	debugText1->font = font;
	debugText1->color = RGBA_WHITE;

	UIElement* debug2 = UICreateElement(NULL);
	debug2->pos = {12, 324};
	debug2->flags = UI_MOVABLE;
	debug2->background = 0x33ffffff;
	UIText* debugText2 = UICreateText(debug2);
	debugText2->font = font;
	debugText2->color = RGBA_WHITE;

	byte buffer[10] = {};

	while(!OSWindowDestroyed() && !OSIsKeyDown(KEY_ESC)) {
		ArenaFreeAll(&scratch);
		OSProcessWindowEvents();

		int32 length1 = Int32ToDecimal(ui.start, buffer);
		debugText1->string = {(byte*)buffer, length1};
		int32 length2 = Int32ToDecimal(ui.end, buffer + length1);
		debugText2->string = {(byte*)buffer+length1, length2};

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