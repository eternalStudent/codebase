struct PCM {
	uint32 size;
	int16* samples;
};

#include "wave.cpp"

#if defined(_WIN32)
#  include "win32audio.cpp"
#  define AudioInit 			Win32AudioInit
#  define AudioPlayMusic		Win32AudioPlayMusic
#  define AudioPlaySound		Win32AudioPlaySound
#  define AudioSetMusicVolume	Win32AudioSetMusicVolume
#  define AudioSetSoundVolume	Win32AudioSetSoundVolume
#  define AudioQueueNext		Win32AudioQueueNext
#endif