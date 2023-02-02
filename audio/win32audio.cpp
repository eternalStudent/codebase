#include <mmdeviceapi.h>
#include <audioclient.h>
#include <initguid.h>
#include <avrt.h>

struct AudioMessage {
	UINT32 type;
	union {UINT32 i; FLOAT f;};
	LPVOID p;
};

#define MESSAGE_QUEUE_CAP		4
#define MESSAGE_QUEUE_MASK		(MESSAGE_QUEUE_CAP - 1)

struct MessageQueue {
	AudioMessage table[MESSAGE_QUEUE_CAP];
	volatile uint32 writeIndex;
	volatile uint32 readIndex;
};

struct {
	IAudioClient3* audioClient;
	MessageQueue queue;
	struct {int16* data; UINT32 length; FLOAT volume; UINT32 mute;} music;
	struct {
		struct {int16* data; UINT32 length;} buckets[32];
		UINT32 availabity;
		FLOAT volume; 
		UINT32 mute;
	} sound;
	struct {int16* data; UINT32 length;} next;
} audio;

void AudioEnqueueMessage(AudioMessage message) {
	audio.queue.table[audio.queue.writeIndex & MESSAGE_QUEUE_MASK] = message;
	_WriteBarrier();
	audio.queue.writeIndex++;
}

BOOL AudioDequeueMessage(AudioMessage* message) {
	if (audio.queue.readIndex == audio.queue.writeIndex) return FALSE;
	*message = audio.queue.table[audio.queue.readIndex & MESSAGE_QUEUE_MASK];
	_ReadWriteBarrier();
	audio.queue.readIndex++;
	return TRUE;
}

void Win32AudioPlayMusic(PCM pcm) {
	AudioMessage message;
	message.type = 1;
	message.p = pcm.samples;
	message.i = pcm.size / 2;
	AudioEnqueueMessage(message);
}

void Win32AudioPlaySound(PCM pcm) {
	AudioMessage message;
	message.type = 2;
	message.p = pcm.samples;
	message.i = pcm.size / 2;
	AudioEnqueueMessage(message);
}

void Win32AudioSetMusicVolume(float32 volume) {
	AudioMessage message;
	message.type = 3;
	message.f = volume;
	AudioEnqueueMessage(message);
}

void Win32AudioSetSoundVolume(float32 volume) {
	AudioMessage message;
	message.type = 4;
	message.f = volume;
	AudioEnqueueMessage(message);
}

void Win32AudioMuteMusic(uint32 value) {
	ASSERT(value == 0 || value == 1);
	AudioMessage message;
	message.type = 5;
	message.i = value;
	AudioEnqueueMessage(message);
}

void Win32AudioMuteSound(uint32 value) {
	ASSERT(value == 0 || value == 1);
	AudioMessage message;
	message.type = 6;
	message.i = value;
	AudioEnqueueMessage(message);
}

void Win32AudioQueueNext(PCM pcm) {
	AudioMessage message;
	message.type = 7;
	message.p = pcm.samples;
	message.i = pcm.size / 2;
	AudioEnqueueMessage(message);
}

DWORD AudioThread(LPVOID arg) {
	DWORD taskIndex;
	HANDLE taskHandle = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);
	ASSERT(taskHandle);
    AvSetMmThreadPriority(taskHandle, AVRT_PRIORITY_CRITICAL);

	IAudioRenderClient* audioRenderClient;
	HRESULT hr = audio.audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&audioRenderClient);
	ASSERT(SUCCEEDED(hr));

	UINT32 numBufferFrames;
	hr = audio.audioClient->GetBufferSize(&numBufferFrames);
	ASSERT(SUCCEEDED(hr));

	HANDLE bufferReady = CreateEvent(NULL, FALSE, FALSE, NULL);
	hr = audio.audioClient->SetEventHandle(bufferReady);
	ASSERT(SUCCEEDED(hr));

	audio.audioClient->Start();
	while (true) {
		WaitForSingleObject(bufferReady, INFINITE);
		AudioMessage message;
		while (AudioDequeueMessage(&message)) {
			if (message.type == 1) {
				audio.music.data = (int16*)message.p;
				audio.music.length = message.i;
				audio.next.data = (int16*)message.p;
				audio.next.length = message.i;
			}
			if (message.type == 2) {
				int32 i = LowBit(~audio.sound.availabity);
				ASSERT(i < 32);
				audio.sound.buckets[i].data = (int16*)message.p;
				audio.sound.buckets[i].length = message.i;
				audio.sound.availabity |= (1 << i);
			}
			if (message.type == 3) {
				audio.music.volume = message.f;
			}
			if (message.type == 4) {
				audio.sound.volume = message.f;
			}
			if (message.type == 5) {
				audio.music.mute = message.i;
			}
			if (message.type == 6) {
				audio.sound.mute = message.i;
			}
			if (message.type == 7) {
				audio.next.data = (int16*)message.p;
				audio.next.length = message.i;
			}
		}

		UINT32 bufferPadding;
		audio.audioClient->GetCurrentPadding(&bufferPadding);

		UINT32 frameCount = numBufferFrames - bufferPadding;
		float32* writeBuffer;
		hr = audioRenderClient->GetBuffer(frameCount, (BYTE**)&writeBuffer);
		ASSERT(SUCCEEDED(hr));

		UINT32 length = frameCount*2;
		for (UINT32 i = 0; i < length; i++) {
			
			float32 music = 0.0f;
			if (!audio.music.mute) {
				music = i < audio.music.length 
					? audio.music.data[i]/32768.0f
					: i - audio.music.length < audio.next.length
						? audio.next.data[i - audio.music.length]/32768.0f
						: 0.0f;
				music *= (audio.music.volume + 1.0f);
			}

			float32 sound = 0.0f;
			if (!audio.sound.mute) for (int32 j = 0; j < 32; j++) {
				sound += i < audio.sound.buckets[j].length 
					? audio.sound.buckets[j].data[i]/32768.0f 
					: 0.0f; 
			}
			sound *= (audio.sound.volume + 1.0f);

			writeBuffer[i] = music + sound;
		}

		if (audio.music.data) {
			if (length < audio.music.length) {
				audio.music.data += length;
				audio.music.length -= length;
			}
			else {
				UINT32 left = length - audio.music.length;
				audio.music.data = audio.next.data + left;
				audio.music.length = audio.next.length - left;
			}
		}

		for (int32 i = 0; i < 32; i++) {
			if (audio.sound.buckets[i].data) {
				uint32 soundLength = MIN(length, audio.sound.buckets[i].length);
				audio.sound.buckets[i].data += soundLength;
				audio.sound.buckets[i].length -= soundLength;
				if (audio.sound.buckets[i].length == 0) {
					audio.sound.availabity &= ~(1 << i);
				}
			}
		}

		audioRenderClient->ReleaseBuffer(frameCount, 0); // releases the previous buffer space
	}

	audio.audioClient->Stop();

	return 0;
}

DWORD InternalAudioInit(LPVOID arg) {
	audio = {};
	CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);

	IMMDeviceEnumerator* deviceEnumerator;
	CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);

	IMMDevice* audioEndPoint;
	deviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &audioEndPoint);

	audioEndPoint->Activate(__uuidof(IAudioClient3), CLSCTX_ALL, NULL, (void**)&audio.audioClient);

	REFERENCE_TIME defaultPeriod; // the default interval between periodic processing passes by the audio engine
	REFERENCE_TIME minimumPeriod; // the minimum interval between periodic processing passes by the audio endpoint device
	audio.audioClient->GetDevicePeriod(&defaultPeriod, &minimumPeriod);	

	WAVEFORMATEX mixFormat;
	mixFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	mixFormat.nChannels = 2;
	mixFormat.nSamplesPerSec = 44100;
	mixFormat.nAvgBytesPerSec = 44100 * 8;
	mixFormat.nBlockAlign = 8; 
	mixFormat.wBitsPerSample = 32;
	mixFormat.cbSize = 0;

	DWORD initStreamFlags = AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
						  | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 
						  | AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

	HRESULT hr = audio.audioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED, 
		initStreamFlags, 
		minimumPeriod, 
		0, 
		&mixFormat, 
		NULL);

	ASSERT(SUCCEEDED(hr));

	if (arg)
		Win32AudioPlayMusic(*(PCM*)arg);
	HANDLE thread = CreateThread(NULL, 0, &AudioThread, NULL, 0, NULL);
	ASSERT(thread);

	return 0;
}

void Win32AudioInit(PCM* pcm) {
	HANDLE thread = CreateThread(NULL, 0, &InternalAudioInit, pcm, 0, NULL);
	ASSERT(thread);
}

