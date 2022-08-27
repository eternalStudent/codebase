#include "basic/basic.cpp"
#include "graphics/graphics.cpp"
#include "ui/ui.cpp"

int main() {
	Arena scratch = CreateArena(1024*1024*1024);
	Arena persist = CreateArena(1024*1024);

	UIInit(&persist, &scratch);
	UICreateWindow("OpenGL Window", {512, 768}, RGBA_DARKGREY);

	Font* font = (Font*)ArenaAlloc(&persist, sizeof(Font));
	*font = LoadFont(&scratch, "..\\consola.ttf", 24);

	Font* smallf = (Font*)ArenaAlloc(&persist, sizeof(Font));
	*smallf = LoadFont(&scratch, "..\\consola.ttf", 18);

	UIElement* e1 = UICreateScrollingPane(NULL, {200, 200}, {12, 12});
	e1->borderWidth = 1;
	e1->borderColor = RGBA_WHITE;
	e1->text.font = font;
	e1->text.color = RGBA_WHITE;
	e1->text.string = STR(R"STRING(Lorem ipsum dolor sit amet, 
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

	UIElement* dropdown = UICreateDropdown(NULL, {98, 36}, {12, 224}, {font, STR("Select"), RGBA_WHITE});
	byte itemsbuf[] = "item 1item 2item 3item 4item 5item 6item 7";
	for (int32 i = 0; i < 7; i++) UIAddDropdownItem(dropdown, {font, {itemsbuf+ 6*i, 6}, RGBA_WHITE});

	UIElement* root = UICreateTreeItem(NULL, {48, 24});
	root->pos = {12, 300};
	UIElement* textElement = UICreateElement(root);
	textElement->x = 18;
	textElement->text = {smallf, STR("root"), RGBA_WHITE};

	UIElement* item1 = UICreateTreeItem(root, {48, 24});
	textElement = UICreateElement(item1);
	textElement->x = 18;
	textElement->text = {smallf, STR("item 1"), RGBA_WHITE};

	UIElement* item2 = UICreateTreeItem(root, {48, 24});
	textElement = UICreateElement(item2);
	textElement->x = 18;
	textElement->text = {smallf, STR("item 2"), RGBA_WHITE};

	UIElement* apple = UICreateTreeItem(item1, {48, 24});
	textElement = UICreateElement(apple);
	textElement->x = 18;
	textElement->text = {smallf, STR("apple"), RGBA_WHITE};

	UIElement* orange = UICreateTreeItem(item1, {48, 24});
	textElement = UICreateElement(orange);
	textElement->x = 18;
	textElement->text = {smallf, STR("orange"), RGBA_WHITE};

	UIElement* abc = UICreateTreeItem(item2, {48, 24});
	textElement = UICreateElement(abc);
	textElement->x = 18;
	textElement->text = {smallf, STR("abc"), RGBA_WHITE};

	UIElement* xyz = UICreateTreeItem(item2, {48, 24});
	textElement = UICreateElement(xyz);
	textElement->x = 18;
	textElement->text = {smallf, STR("xyz"), RGBA_WHITE};


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