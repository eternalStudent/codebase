#define LOG(text)	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), text, sizeof(text)-1, NULL, NULL)

#include "basic/basic.cpp"
#include "graphics/graphics.cpp"
#include "ui/ui.cpp"
#include "audio/audio.cpp"

void Play(UIElement* e) {
	OSAudioPlaySound((PCM*)e->context.p);
}

void SetVolume1(UIElement* e) {
	float32 value = (float32)e->x*LoadExp(-6) - 1.0f;
	OSAudioSetMusicVolume(value);
}

void SetVolume2(UIElement* e) {
	float32 value = (float32)e->x*LoadExp(-6) - 1.0f;
	OSAudioSetSoundVolume(value);
}

int main() {
	Arena scratch = CreateArena(1024*1024*1024);
	Arena persist = CreateArena(1024*1024*1024);

	UIInit(&persist, &scratch);
	UICreateWindow("blah blah blah", {512, 768}, RGBA_DARKGREY);
	Win32AudioInit();

	File file1 = Win32OpenFile(L"..\\wasapi\\data\\Nitemare.wav");
	byte* data1 = OSReadAll(file1, &scratch).data;
	ASSERT(data1);
	PCM* wav1 = WAVLoadAudio(&persist, data1);

	UIElement* button1 = UICreateElement(NULL);
	button1->dim = {120, 60};
	button1->pos = {30, 30};
	button1->onClick = Play;
	button1->flags = UI_CLICKABLE;
	button1->background = RGBA_ORANGE;
	button1->context.p = wav1;

	File file2 = Win32OpenFile(L"..\\wasapi\\data\\blink.wav");
	byte* data2 = OSReadAll(file2, &scratch).data;
	ASSERT(data2);
	PCM* wav2 = WAVLoadAudio(&persist, data2);

	UIElement* button2 = UICreateElement(NULL);
	button2->dim = { 120, 60 };
	button2->pos = { 180, 30 };
	button2->onClick = Play;
	button2->flags = UI_CLICKABLE;
	button2->background = RGBA_ORANGE;
	button2->context.p = wav2;


	File file3 = Win32OpenFile(L"..\\wasapi\\data\\main theme1.wav");
	byte* data3 = OSReadAll(file3, &scratch).data;
	ASSERT(data3);
	PCM* theme = WAVLoadAudio(&persist, data3);
	Win32AudioPlayMusic(theme);

	UIElement* bloop = UICreateSlider(NULL, 128+8);
	bloop->pos = { 120, 120 };
	bloop->first->onMove = SetVolume1;

	UIElement* bleep = UICreateSlider(NULL, 128+8);
	bleep->pos = { 120, 250 };
	bleep->first->onMove = SetVolume2;
	
	while(!OSWindowDestroyed()) {
		if (OSIsKeyPressed(KEY_ESC)) break;

		ArenaFreeAll(&scratch);
		OSProcessWindowEvents();

		GfxClearScreen();

		UIUpdateActiveElement();
		UIRenderElements();

		GfxSwapBuffers();
	}
}

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
extern "C" void WinMainCRTStartup() {
    ExitProcess(main());
}
#endif