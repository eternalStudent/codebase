struct PCM {
	uint32 size;
	int16 samples;
};

#include "wave.cpp"

#if defined(_WIN32)
#  include "win32audio.cpp"
#  define OSAudioInit				Win32AudioInit
#  define OSAudioPlayMusic			Win32AudioPlayMusic
#  define OSAudioPlaySound			Win32AudioPlaySound
#  define OSAudioSetMusicVolume 	Win32AudioSetMusicVolume
#  define OSAudioSetSoundVolume 	Win32AudioSetSoundVolume
#endif