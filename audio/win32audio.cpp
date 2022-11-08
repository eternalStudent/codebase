#include <mmdeviceapi.h>
#include <audioclient.h>
#include <initguid.h>
#include <avrt.h>

struct AudioMessage {
	UINT32 type;
	UINT32 num;
	LPVOID ptr;
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
	struct {int16* data; UINT32 length;} music;
	struct {int16* data; UINT32 length;} sound;
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

void Win32AudioPlayMusic(PCM* pcm) {
	AudioMessage message;
	message.type = 1;
	message.ptr = &pcm->samples;
	message.num = pcm->size / 2;
	AudioEnqueueMessage(message);
}

void Win32AudioPlaySound(PCM* pcm) {
	AudioMessage message;
	message.type = 2;
	message.ptr = &pcm->samples;
	message.num = pcm->size / 2;
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
				audio.music.data = (int16*)message.ptr;
				audio.music.length = message.num;
			}
			if (message.type == 2) {
				audio.sound.data = (int16*)message.ptr;
				audio.sound.length = message.num;
			}
		}

		UINT32 bufferPadding;
		audio.audioClient->GetCurrentPadding(&bufferPadding);

		UINT32 frameCount = numBufferFrames - bufferPadding;
		int16* writeBuffer;
		hr = audioRenderClient->GetBuffer(frameCount, (BYTE**)&writeBuffer);
		ASSERT(SUCCEEDED(hr));

		for (UINT32 i = 0; i < frameCount*2; i++) {
			int16 music = i < audio.music.length ? audio.music.data[i] : 0; 
			int16 sound = i < audio.sound.length ? audio.sound.data[i] : 0; 
			writeBuffer[i] = music + sound;
		}
		uint32 musicLength = MIN(frameCount*2, audio.music.length);
		audio.music.data += musicLength;
		audio.music.length -= musicLength;

		uint32 soundLength = MIN(frameCount*2, audio.sound.length);
		audio.sound.data += soundLength;
		audio.sound.length -= soundLength;

		audioRenderClient->ReleaseBuffer(frameCount, 0); // releases the previous buffer space
	}

	audio.audioClient->Stop();

	return 0;
}

void Win32AudioInit() {
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
	mixFormat.wFormatTag = WAVE_FORMAT_PCM;
	mixFormat.nChannels = 2;
	mixFormat.nSamplesPerSec = 44100;
	mixFormat.nAvgBytesPerSec = 44100 * 4;
	mixFormat.nBlockAlign = 4;
	mixFormat.wBitsPerSample = 16;
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

	HANDLE thread = CreateThread(NULL, 0, &AudioThread, NULL, 0, NULL);
    ASSERT(thread);
}
