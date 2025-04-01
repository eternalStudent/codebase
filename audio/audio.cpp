struct PCM {
	uint32 size;
	int16* samples;
};

#include "wave.cpp"

#if defined(OS_WINDOWS)
#  include "win32audio.cpp"
#  define AudioInit 			Win32AudioInit
#  define AudioPlayMusic		Win32AudioPlayMusic
#  define AudioPlaySound		Win32AudioPlaySound
#  define AudioSetMusicVolume	Win32AudioSetMusicVolume
#  define AudioSetSoundVolume	Win32AudioSetSoundVolume
#  define AudioQueueNext		Win32AudioQueueNext
#elif defined(OS_LINUX)
#  include "linuxaudio.cpp"
#  define AudioInit 			LinuxAudioInit
#  define AudioPlayMusic		LinuxAudioPlayMusic
#  define AudioPlaySound		LinuxAudioPlaySound
#  define AudioSetMusicVolume	LinuxAudioSetMusicVolume
#  define AudioSetSoundVolume	LinuxAudioSetSoundVolume
#  define AudioQueueNext		LinuxAudioQueueNext
#endif