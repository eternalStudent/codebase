#include "basic/basic.cpp"
#include "graphics/graphics.cpp"
#include "ui/ui.cpp"

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
void __stdcall WinMainCRTStartup()
{
    ExitProcess(0);
}
#else
int main() {
	return 0;
}
#endif
