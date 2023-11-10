void* LinuxAllocate(size_t size) {
	return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void* LinuxReserve(size_t size) {
	return mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

int LinuxCommit(void* data, size_t size) {
	return mprotect(data, size, PROT_READ | PROT_WRITE);
}

int LinuxFree(void* data, size_t size) {
	return munmap(data, size);
}

void LinuxDecommit(void* data, size_t size) {
	mprotect(data, size, PROT_NONE);
	madvise(data, size, MADV_FREE);
}