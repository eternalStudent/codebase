void* LinuxHeapAllocate(size_t size) {
	return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

int LinuxHeapFree(void* data, size_t size) {
	return munmap(data, size);
}