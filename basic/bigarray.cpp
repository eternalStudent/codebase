struct BigArray {
	byte* data;
	ssize size;
	ssize capacity;
};

BigArray CreateBigArray() {
	BigArray arr = {};
	arr.data = (byte*)OSReserve(RESERVE_SIZE);
	OSCommit(arr.data, CHUNK_SIZE);
	arr.capacity = CHUNK_SIZE;
	arr.size = 0;
	return arr;
}

void BigArrayAdd(BigArray* array, void* data, ssize size) {
	while (array->capacity < array->size + size) {
		OSCommit(array->data + array->capacity, CHUNK_SIZE);
		array->capacity += CHUNK_SIZE;
	}

	memcpy(array->data + array->size, data, size);
	array->size += size;
}

void DestroyBigArray(BigArray* arr) {
	OSFree(arr->data, RESERVE_SIZE);
}

#define BIGARRAY_FOREACH(arr, t, n) 	for (t* n = (t*)arr->data; n < (t*)(arr->data + arr->size); n++)

struct BigBuffer {
	byte* start;
	byte* pos;
	byte* end;
};

BigBuffer CreateBigBuffer() {
	BigBuffer buffer = {};
	buffer.start = (byte*)OSReserve(RESERVE_SIZE);
	OSCommit(buffer.start, CHUNK_SIZE);
	buffer.end = buffer.start + CHUNK_SIZE;
	buffer.pos = buffer.start;
	return buffer;
}

void BigBufferEnsureCapacity(BigBuffer* buffer, ssize length) {
	// NOTE: unlike BigArray, I want to make sure there is always extra capacity
	//       the `while` is slightly unnecessary, but why not
	while (buffer->end < buffer->pos + length + 20) {
		OSCommit(buffer->end, CHUNK_SIZE);
		buffer->end += CHUNK_SIZE;
	}
}

void BigBufferWrite(BigBuffer* buffer, byte* data, ssize length) {
	BigBufferEnsureCapacity(buffer, length);
	memcpy(buffer->pos, data, length);
	buffer->pos += length;
}

void BigBufferWrite(BigBuffer* buffer, String str) {
	BigBufferWrite(buffer, str.data, str.length);
}

// NOTE: same as BigBufferWrite, but does not update `pos`
ssize StringCopy(String source, BigBuffer* buffer) {
	BigBufferEnsureCapacity(buffer, source.length);
	return StringCopy(source, buffer->pos);
}

void BigBufferReset(BigBuffer* buffer) {
	buffer->pos = buffer->start;
}

void DestroyBigBuffer(BigBuffer* buffer) {
	OSFree(buffer->start, RESERVE_SIZE);
}
