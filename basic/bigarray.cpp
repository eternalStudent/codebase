struct BigArray {
	byte* data;
	ssize size;
	ssize capacity;
	
	ssize commitSize;
};

BigArray CreateBigArray(ssize commitSize) {
	BigArray arr = {};
	arr.data = (byte*)OSReserve(RESERVE_SIZE);
	OSCommit(arr.data, commitSize);
	arr.capacity = commitSize;
	arr.commitSize = commitSize;
	arr.size = 0;
	return arr;
}

void BigArrayPush(BigArray* arr, void* data, ssize size) {
	while (arr->capacity < arr->size + size) {
		OSCommit(arr->data + arr->capacity, arr->commitSize);
		arr->capacity += arr->commitSize;
	}

	memcpy(arr->data + arr->size, data, size);
	arr->size += size;
}

void* BigArrayPop(BigArray* arr, ssize size) {
	arr->size -= size;
	return arr->data + arr->size;
}

void BigArrayClear(BigArray* arr) {
	arr->size = 0;
}

void DestroyBigArray(BigArray* arr) {
	OSFree(arr->data, RESERVE_SIZE);
}

#define BIGARRAY_FOREACH(arr, t, n) 	for (t* n = (t*)arr->data; n < (t*)(arr->data + arr->size); n++)

struct BigBuffer {
	byte* start;
	byte* pos;
	byte* end;

	ssize commitSize;
};

BigBuffer CreateBigBuffer(ssize commitSize) {
	BigBuffer buffer = {};
	buffer.start = (byte*)OSReserve(RESERVE_SIZE);
	OSCommit(buffer.start, commitSize);
	buffer.end = buffer.start + commitSize;
	buffer.pos = buffer.start;
	buffer.commitSize = commitSize;
	return buffer;
}

void BigBufferEnsureCapacity(BigBuffer* buffer, ssize length) {
	// NOTE: unlike BigArray, I want to make sure there is always extra capacity
	//       the `while` is slightly unnecessary, but why not
	while (buffer->end < buffer->pos + length + 20) {
		OSCommit(buffer->end, buffer->commitSize);
		buffer->end += buffer->commitSize;
	}
}

String BigBufferWrite(BigBuffer* buffer, byte* data, ssize length) {
	BigBufferEnsureCapacity(buffer, length);
	memcpy(buffer->pos, data, length);
	String str = {buffer->pos, length};
	buffer->pos += length;
	return str;
}

String BigBufferWrite(BigBuffer* buffer, String str) {
	return BigBufferWrite(buffer, str.data, str.length);
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
