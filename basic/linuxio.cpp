int LinuxOpenFile(const char* filePath) {
    int fd = open(
        filePath, 
        O_RDWR
    ); 
    if (fd == -1) {
        LOG("failed to open file");
    }
    return fd;
}

int LinuxCreateFile(const char* filePath) {
    int fd = open(
        filePath, 
        O_CREAT|O_RDWR
    ); 
    if (fd == -1) {
        LOG("failed to open file");
    }
    return fd;
}

ssize_t LinuxReadFile(int fd, void* buffer, ssize_t maxBytesToRead) {
    ssize_t bytesRead = read(fd, buffer, maxBytesToRead);
    if (bytesRead == -1) {
        LOG("failed to read file");
    }
    return bytesRead;
}

ssize_t LinuxWriteFile(int fd, void* buffer, ssize_t maxBytesToWrite){
    ssize_t bytesWritten = write(fd, buffer, maxBytesToWrite);
    if (bytesWritten == -1) {
        LOG("failed to write to file");
    }
    return bytesWritten;
}

off_t LinuxGetFileSize(int fd) {
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}