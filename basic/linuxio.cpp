int LinuxOpenFile(const char* filePath) {
    int fd = open(
        filePath, 
        O_RDWR
    ); 
    if (fd == -1 && errno == EACCES) {
        fd = open(filePath, O_RDONLY); 
        if (fd == -1) {
            LOG("failed to open file");
        }
    }
    return fd;
}

int LinuxCreateFile(const char* filePath) {
    int fd = open(
        filePath, 
        O_CREAT, O_RDWR
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

#if defined(__gnu_linux__)
#include <fontconfig/fontconfig.h>
int LinuxGetDefaultFontFile() {
    FcPattern* pattern = FcNameParse((const FcChar8*)"monospace");
    FcBool success = FcConfigSubstitute(0,pattern,FcMatchPattern);
    if (!success) {
        LOG("failed to find font");
        return {};
    }
    
    FcDefaultSubstitute(pattern);
    FcResult result;
    FcPattern* fontMatch = FcFontMatch(NULL, pattern, &result);
    if (!fontMatch) {
        LOG("failed to match font");
        return {};
    }

    FcChar8 *filename;
    if (FcPatternGetString(fontMatch, FC_FILE, 0, &filename) != FcResultMatch) {
        LOG("failed to get font file name");
        return {};
    }

    return LinuxOpenFile((const char*)filename);
}
#endif