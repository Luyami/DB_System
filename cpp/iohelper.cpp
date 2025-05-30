#include <iostream>
#include <fstream>
#include <string>

#include "iohelper.h"

bool IOHelper::file_exists(const char* filename) {
    std::ifstream f(filename);
    return f.good();
}

bool IOHelper::file_create(const char* filename) {
    if (file_exists(filename)) return false; //File already exists, so we don't have to create it again!

    std::ofstream file(filename);
    
    return file ? true : false;
}

bool IOHelper::file_write(const char* filename, const char* data){
    if (!file_exists(filename) && !file_create(filename)) return false;

    std::ofstream file(filename, std::ios::app);
    
    if (file){
        file << data;
        file.flush();
        return true;
    }
    else return false;
}

bool IOHelper::file_write(const char* filename, const char* data, long long count){
    if (!file_exists(filename) && !file_create(filename)) return false;

std::ofstream file(filename, std::ios::binary | std::ios::in | std::ios::out | std::ios::ate);
    
    if (file){
        file.write(data, count);
        file.flush();
        return true;
    }
    else return false;
}

bool IOHelper::file_write(const char* filename, const char* data, long long count, long long startIdx){
    if (!file_exists(filename) && !file_create(filename)) return false;

    std::ofstream file(filename, std::ios::in | std::ios::out | std::ios::binary);
    
    if (file){
        file.seekp(startIdx, std::ios::beg);
        file.write(data, count);
        file.flush();
        return true;
    }
    else return false;
}

bool IOHelper::file_read(const char* filename, const size_t bytes_to_read, char* buffer, size_t* read_bytes){
    //sizeof(ready_bytes) must match bytes_to_read

    if (!file_exists(filename) && !file_create(filename)) return false;

    std::ifstream file(filename, std::ios::binary);

    if (file){
        file.read(buffer, bytes_to_read);

        *read_bytes = file.gcount();
        return true;
    } else return false;
}

bool IOHelper::file_read(const char* filename, const size_t bytes_to_read, char* buffer, size_t* read_bytes, long long startIdx){
    //sizeof(ready_bytes) must match bytes_to_read

    if (!file_exists(filename) && !file_create(filename)) return false;

    std::ifstream file(filename, std::ios::binary);
    file.seekg(startIdx);

    if (file){
        file.read(buffer, bytes_to_read);

        *read_bytes = file.gcount();
        return true;
    } else return false;
}

bool IOHelper::file_getLine(const char* filename, int lineIdx, std::string* lineBuffer){
    if (!file_exists(filename) && !file_create(filename)) return false;
    std::ifstream file(filename);

    for (int i = 0; i < lineIdx; ++i){
        std::getline(file, *lineBuffer); //skipping all other lines
    }
    std::getline(file, *lineBuffer);

    return true;
}


long long IOHelper::file_size(const char* filename){
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    return file ? (long long) file.tellg() : -1;
}