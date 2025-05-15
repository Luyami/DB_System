#ifndef IOHELPER_H
#define IOHELPER_H

namespace IOHelper{
    bool file_exists(const char* filename);
    bool file_create(const char* filename);
    bool file_write(const char* filename, const char* data);
    bool file_write(const char* filename, const char* data, long long count);
    bool file_write(const char* filename, const char* data, long long count, long long startIdx);
    bool file_read(const char* filename, const size_t bytes_to_read, char* buffer, size_t* read_bytes);
    bool file_read(const char* filename, const size_t bytes_to_read, char* buffer, size_t* read_bytes, long long startIdx);
    bool file_getLine(const char* filename, int lineIdx, std::string* lineBuffer);
    long long file_size(const char* filename);
};

#endif