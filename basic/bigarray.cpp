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
