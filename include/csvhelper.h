#ifndef CSVHELPER_H
#define CSVHELPER_H

#include <string>
#include <vector>

#include "iohelper.h"

using namespace std;

namespace CSV{
    struct CSVInfos{
        vector<string> fields;
        vector<vector<string>> data;
    };

    size_t getRowSizeEstimativeInBytes(const char* filename); //An estimative based on the first data row's size
    vector<string> splitRow(string rawRow);
    CSVInfos readChunk(const char* filename, unsigned int dataRowsCount);
};

#endif