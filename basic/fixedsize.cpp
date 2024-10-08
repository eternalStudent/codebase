struct FixedSize {
	byte* buffer;
	ssize capacity;
	byte* next;
	ssize sizeofItem;

	ssize commitSize;
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

FixedSize CreateFixedSize(ssize sizeofItem, ssize commitSize) {
	commitSize = (commitSize/sizeofItem)*sizeofItem; // Remove the remainder
	FixedSize allocator = {};
	allocator.buffer = (byte*)OSReserve(RESERVE_SIZE);
	allocator.capacity = commitSize; 
	allocator.sizeofItem = sizeofItem;
	allocator.next = allocator.buffer;
	allocator.commitSize = commitSize;

	OSCommit(allocator.buffer, allocator.capacity);
	FixedSizeReset(&allocator);

	return allocator;
}

void Grow(FixedSize* allocator) {
	OSCommit(allocator->buffer + allocator->capacity, allocator->commitSize);

	allocator->next = allocator->buffer + allocator->capacity;

	for (byte* buffer = allocator->buffer + allocator->capacity; 
		 buffer + allocator->sizeofItem < allocator->buffer + allocator->capacity + allocator->commitSize; 
		 buffer += allocator->sizeofItem) {

		*(byte**)buffer = buffer + allocator->sizeofItem;
	}

	allocator->capacity += allocator->commitSize;
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