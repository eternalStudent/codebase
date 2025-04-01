#include <alsa/asoundlib.h>

struct AudioMessage {
	uint32 type;
	union {uint32 i; float32 f;};
	void* p;
};

#define MESSAGE_QUEUE_CAP		4
#define MESSAGE_QUEUE_MASK		(MESSAGE_QUEUE_CAP - 1)

struct MessageQueue {
	AudioMessage table[MESSAGE_QUEUE_CAP];
	volatile uint32 writeIndex;
	volatile uint32 readIndex;
};

struct {
	snd_pcm_t* handle;
	snd_pcm_uframes_t periodSize;
	unsigned int periods;

	MessageQueue queue;
	struct {int16* data; uint32 length; float32 volume; uint32 mute;} music;
	struct {
		struct {int16* data; uint32 length;} buckets[32];
		uint32 availabity;
		float32 volume; 
		uint32 mute;
	} sound;
	struct {int16* data; uint32 length;} next;
} audio;

void AudioEnqueueMessage(AudioMessage message) {
	audio.queue.table[audio.queue.writeIndex & MESSAGE_QUEUE_MASK] = message;
	__asm__ __volatile__ ("":::"memory");
	audio.queue.writeIndex++;
}

bool AudioDequeueMessage(AudioMessage* message) {
	if (audio.queue.readIndex == audio.queue.writeIndex) return false;
	*message = audio.queue.table[audio.queue.readIndex & MESSAGE_QUEUE_MASK];
	__asm__ __volatile__ ("":::"memory");
	audio.queue.readIndex++;
	return true;
}

void LinuxAudioPlayMusic(PCM pcm) {
	AudioMessage message;
	message.type = 1;
	message.p = pcm.samples;
	message.i = pcm.size / 2;
	AudioEnqueueMessage(message);
}

void LinuxAudioPlaySound(PCM pcm) {
	AudioMessage message;
	message.type = 2;
	message.p = pcm.samples;
	message.i = pcm.size / 2;
	AudioEnqueueMessage(message);
}

void LinuxAudioSetMusicVolume(float32 volume) {
	AudioMessage message;
	message.type = 3;
	message.f = volume;
	AudioEnqueueMessage(message);
}

void LinuxAudioSetSoundVolume(float32 volume) {
	AudioMessage message;
	message.type = 4;
	message.f = volume;
	AudioEnqueueMessage(message);
}

void LinuxAudioMuteMusic(uint32 value) {
	ASSERT(value == 0 || value == 1);
	AudioMessage message;
	message.type = 5;
	message.i = value;
	AudioEnqueueMessage(message);
}

void LinuxAudioMuteSound(uint32 value) {
	ASSERT(value == 0 || value == 1);
	AudioMessage message;
	message.type = 6;
	message.i = value;
	AudioEnqueueMessage(message);
}

void LinuxAudioQueueNext(PCM pcm) {
	AudioMessage message;
	message.type = 7;
	message.p = pcm.samples;
	message.i = pcm.size / 2;
	AudioEnqueueMessage(message);
}

static int xrun_recovery(int error) {
	if (error == -EPIPE) {    /* under-run */
		error = snd_pcm_prepare(audio.handle);
		ASSERT(error == 0);

		return 0;
	} 
	else if (error == -ESTRPIPE) {
		while ((error = snd_pcm_resume(audio.handle)) == -EAGAIN)
			sleep(1);   /* wait until the suspend flag is released */
		if (error < 0) {
			error = snd_pcm_prepare(audio.handle);
			ASSERT(error == 0);
		}
		return 0;
	}
	return error;
}

void* AudioThread(void* arg) {
	int error;
	const snd_pcm_channel_area_t *areas;
	snd_pcm_uframes_t offset, frames;
	snd_pcm_sframes_t avail, commitres;
	snd_pcm_state_t state;
	int first = 1;
 
	while (1) {
		state = snd_pcm_state(audio.handle);
		if (state == SND_PCM_STATE_XRUN) {
			error = xrun_recovery(-EPIPE);
			ASSERT(error == 0);

			first = 1;
		} else if (state == SND_PCM_STATE_SUSPENDED) {
			error = xrun_recovery(-ESTRPIPE);
			ASSERT(error == 0);
		}
		avail = snd_pcm_avail_update(audio.handle);
		if (avail < 0) {
			error = xrun_recovery(avail);
			ASSERT(error == 0);
			first = 1;
			continue;
		}
		if (avail < audio.periodSize) {
			if (first) {
				first = 0;
				error = snd_pcm_start(audio.handle);
				ASSERT(error == 0);
			} else {
				error = snd_pcm_wait(audio.handle, -1);
				if (error < 0) {
					error = xrun_recovery(error);
					ASSERT(error == 0);
					first = 1;
				}
			}
			continue;
		}

		AudioMessage message;
		while (AudioDequeueMessage(&message)) {
			if (message.type == 1) {
				audio.music.data = (int16*)message.p;
				audio.music.length = message.i;
				audio.next.data = (int16*)message.p;
				audio.next.length = message.i;
			}
			if (message.type == 2) {
				int32 i = (int32)LowBit(~audio.sound.availabity);
				if (i == 32) {
					audio.sound.availabity &= ~1;
					i = 0;
				}
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

		snd_pcm_uframes_t size = audio.periodSize;
		while (size > 0) {
			frames = size;
			error = snd_pcm_mmap_begin(audio.handle, &areas, &offset, &frames);
			if (error < 0) {
				error = xrun_recovery(error);
				ASSERT(error == 0);
				first = 1;
			}

			float32* buffer = (float32*)((byte*)areas[0].addr + (areas[0].first/8) + offset*areas[0].step/8);
			uint32 length = 2*frames;
			for (uint32 i = 0; i < length; i++) {
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

				buffer[i] = music + sound;
			}
			
			if (audio.music.data) {
				if (length < audio.music.length) {
					audio.music.data += length;
					audio.music.length -= length;
				}
				else {
					uint32 left = length - audio.music.length;
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

			commitres = snd_pcm_mmap_commit(audio.handle, offset, frames);
			if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
				error = xrun_recovery(commitres >= 0 ? -EPIPE : commitres);
				ASSERT(error == 0);
				first = 1;
			}
			size -= frames;
		}
	}
 
	snd_pcm_close(audio.handle);

	return NULL;
}

void* AudioInitThread(void* arg) {
	const char *device = "plughw:0,0";                 /* playback device */
	unsigned int buffer_time = 500000;                 /* ring buffer length in us */
	unsigned int period_time = 100000;                 /* period time in us */
	int resample = 1;                                  /* enable alsa-lib resampling */
	int period_event = 0;                              /* produce poll event after each period */
	unsigned int rate = 44100;           			   /* stream rate */
	snd_pcm_format_t format = SND_PCM_FORMAT_FLOAT;    /* sample format */
	unsigned int channels = 2; 

	int error;
	audio = {};

	snd_output_t *output;
	error = snd_output_stdio_attach(&output, stdout, 0);
	ASSERT(error == 0);
	
	error = snd_pcm_open(&audio.handle, device, SND_PCM_STREAM_PLAYBACK, 0);
	ASSERT(error == 0);
	
	/* choose all parameters */
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_hw_params_alloca(&hwparams);
	error = snd_pcm_hw_params_any(audio.handle, hwparams);
	ASSERT(error == 0);

	/* set hardware resampling */
	error = snd_pcm_hw_params_set_rate_resample(audio.handle, hwparams, resample);
	ASSERT(error == 0);

	/* set the interleaved read/write format */
	error = snd_pcm_hw_params_set_access(audio.handle, hwparams, SND_PCM_ACCESS_MMAP_INTERLEAVED);
	ASSERT(error == 0);

	/* set the sample format */
	error = snd_pcm_hw_params_set_format(audio.handle, hwparams, format);
	ASSERT(error == 0);

	/* set the count of channels */
	error = snd_pcm_hw_params_set_channels(audio.handle, hwparams, channels);
	ASSERT(error == 0);

	/* set the stream rate */
	unsigned int rrate = rate;
	error = snd_pcm_hw_params_set_rate_near(audio.handle, hwparams, &rrate, 0);
	ASSERT(error == 0 && rrate == rate);

	/* set the buffer time */
	int dir;
	error = snd_pcm_hw_params_set_buffer_time_near(audio.handle, hwparams, &buffer_time, &dir);
	ASSERT(error == 0);

	snd_pcm_uframes_t buffer_size;
	error = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
	ASSERT(error == 0);

	/* set the period time */
	error = snd_pcm_hw_params_set_period_time_near(audio.handle, hwparams, &period_time, &dir);
	ASSERT(error == 0);

	audio.periodSize = buffer_size;
	error = snd_pcm_hw_params_get_period_size(hwparams, &audio.periodSize, &dir);
	ASSERT(error == 0);

	/* write the parameters to device */
	error = snd_pcm_hw_params(audio.handle, hwparams);
	ASSERT(error == 0);
	
	snd_pcm_sw_params_t *swparams;
	snd_pcm_sw_params_alloca(&swparams);
	error = snd_pcm_sw_params_current(audio.handle, swparams);
	ASSERT(error == 0);

	/* start the transfer when the buffer is almost full: */
	/* (buffer_size / avail_min) * avail_min */
	error = snd_pcm_sw_params_set_start_threshold(audio.handle, swparams, (buffer_size / audio.periodSize) * audio.periodSize);
	ASSERT(error == 0);

	/* allow the transfer when at least audio.periodSize samples can be processed */
	/* or disable this mechanism when period event is enabled (aka interrorupt like style processing) */
	error = snd_pcm_sw_params_set_avail_min(audio.handle, swparams, period_event ? buffer_size : audio.periodSize);
	ASSERT(error == 0);

	/* enable period events when requested */
	if (period_event) {
		error = snd_pcm_sw_params_set_period_event(audio.handle, swparams, 1);
		ASSERT(error == 0);
	}

	/* write the parameters to the playback device */
	error = snd_pcm_sw_params(audio.handle, swparams);
	ASSERT(error == 0);

	if (arg) {
		PCM* pcm = (PCM*)arg;
		LinuxAudioPlayMusic(*pcm);
	}

	AudioThread(NULL);

	return NULL;
}

void LinuxAudioInit(PCM* pcm) {
	pthread_t thread;
	int error = pthread_create(&thread, NULL, AudioInitThread, pcm);
	ASSERT(error == 0);
}