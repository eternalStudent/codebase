struct FixedSize {
	void* buffer;
	void* next;
	ssize size;
	int32 cap;
};

void FixedSizeReset(FixedSize* allocator) {
	byte* buffer = (byte*)allocator->buffer;
	allocator->next = buffer;

	for (int32 i = 0; i < allocator->cap-2; i++) {
		*(void**)buffer = (void*)(buffer + allocator->size);
		buffer = *(byte**)buffer;
	}
	*(void**)buffer = NULL;
}

void FixedSizeInit(FixedSize* allocator, byte* buffer, int32 cap, ssize size) {
	allocator->buffer = buffer;
	allocator->size = size;
	allocator->cap = cap;
	
	FixedSizeReset(allocator);
}

void* FixedSizeAlloc(FixedSize*  allocator) {
	if (allocator->next == NULL) return NULL;
	void* data = allocator->next;
	allocator->next = *(void**)data;
	memset(data, 0, allocator->size);
	return data;
}

void FixedSizeFree(FixedSize* allocator, void* data) {
	if (data == NULL) return;
	*(void**)data = allocator->next;
	allocator->next = data;
}

int32 FixedSizeGetIndex(FixedSize* allocator, void* data) {
	return (int32)(((byte*)data - (byte*)allocator->buffer)/allocator->size);
}