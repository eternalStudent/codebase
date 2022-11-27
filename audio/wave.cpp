PCM WAVLoadAudio(byte* data) {
	uint32 RIFF = ReadUint32LittleEndian(&data);
	ASSERT(RIFF == 1179011410);
	uint32 size = ReadUint32LittleEndian(&data);
	uint32 WAVE = ReadUint32LittleEndian(&data);
	ASSERT(WAVE == 1163280727);
	uint32 fmt_ = ReadUint32LittleEndian(&data);
	ASSERT(fmt_ == 544501094);
	size = ReadUint32LittleEndian(&data);
	uint16 formatTag = ReadUint16LittleEndian(&data);
	ASSERT(formatTag == 1); // PCM
	uint16 channels = ReadUint16LittleEndian(&data);
	ASSERT(channels == 2);
	uint32 samplesPerSec = ReadUint32LittleEndian(&data);
	ASSERT(samplesPerSec == 44100);
	uint32 bytesPerSec = ReadUint32LittleEndian(&data);
	ASSERT(bytesPerSec == 44100*4);
	uint16 alignment = ReadUint16LittleEndian(&data);
	ASSERT(alignment == 4);
	uint16 bitsPerSample = ReadUint16LittleEndian(&data);
	ASSERT(bitsPerSample == 16);
	uint32 section = ReadUint32LittleEndian(&data);
	if (section == 1414744396) { // LIST
		size = ReadUint32LittleEndian(&data);
		Skip(&data, size);
		section = ReadUint32LittleEndian(&data);
	}

	ASSERT(section == 1635017060); // data
	size = ReadUint32LittleEndian(&data);
	PCM pcm = {size, (int16*)data};
	return pcm;
}