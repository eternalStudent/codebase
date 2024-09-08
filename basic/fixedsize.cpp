struct FixedSize {
	byte* buffer;
	ssize capacity;
	byte* next;
	ssize sizeofItem;
};

void FixedSizeReset(FixedSize* allocator) {
	allocator->next = allocator->buffer;

	for (byte* buffer = allocator->buffer; 
		 buffer + allocator->sizeofItem < allocator->buffer + allocator->capacity; 
		 buffer += allocator->sizeofItem) {

		*(byte**)buffer = buffer + allocator->sizeofItem;
	}

	*(byte**)(allocator->buffer + allocator->capacity - allocator->sizeofItem) = NULL;
}

FixedSize CreateFixedSize(ssize sizeofItem) {
	FixedSize allocator = {};
	allocator.buffer = (byte*)OSReserve(RESERVE_SIZE);
	allocator.capacity = (CHUNK_SIZE/sizeofItem) * sizeofItem; // Remove the remainder
	allocator.sizeofItem = sizeofItem;
	allocator.next = allocator.buffer;

	OSCommit(allocator.buffer, allocator.capacity);
	FixedSizeReset(&allocator);

	return allocator;
}

void Grow(FixedSize* allocator) {
	ssize extraCapacity = (CHUNK_SIZE/allocator->sizeofItem) * allocator->sizeofItem;
	OSCommit(allocator->buffer + allocator->capacity, extraCapacity);

	allocator->next = allocator->buffer + allocator->capacity - allocator->sizeofItem;

	for (byte* buffer = allocator->buffer + allocator->capacity - allocator->sizeofItem; 
		 buffer + allocator->sizeofItem < allocator->buffer + allocator->capacity + extraCapacity; 
		 buffer += allocator->sizeofItem) {

		*(byte**)buffer = buffer + allocator->sizeofItem;
	}

	allocator->capacity += extraCapacity;
	*(byte**)(allocator->buffer + allocator->capacity - allocator->sizeofItem) = NULL;
}

void* FixedSizeAlloc(FixedSize*  allocator) {
	if (allocator->next == NULL) Grow(allocator);

	byte* data = allocator->next;
	allocator->next = *(byte**)data;
	memset(data, 0, allocator->sizeofItem);
	return data;
}

void FixedSizeFree(FixedSize* allocator, void* data) {
	if (data == NULL) return;
	*(byte**)data = allocator->next;
	allocator->next = (byte*)data;
}

ssize FixedSizeGetIndex(FixedSize* allocator, void* data) {
	return (ssize)(((byte*)data - allocator->buffer)/allocator->sizeofItem);
}

void DestroyFixedSize(FixedSize* allocator) {
	OSFree(allocator->buffer, RESERVE_SIZE);
}