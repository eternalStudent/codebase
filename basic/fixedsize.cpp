struct FixedSize {
	void* buffer; // mainy for debugging
	void* next;
	ssize size;
};

void FixedSizeInit(FixedSize* allocator, byte* buffer, ssize cap, ssize size) {
	allocator->buffer = buffer;
	allocator->next = buffer;
	allocator->size = size;

	for (ssize i = 0; i < cap-2; i++) {
		*(void**)buffer = (void*)(buffer + size);
		buffer = *(byte**)buffer;
	}
	*(void**)buffer = NULL;
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