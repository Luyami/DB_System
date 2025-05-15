#include "csvhelper.h"
#include <iostream>

#define READ_CHUNK 8000//in bytes

size_t CSV::getRowSizeEstimativeInBytes(const char* filename){
    std::string lineBuffer;
    
    IOHelper::file_getLine(filename, 1, &lineBuffer); //1 is the index for the first data row
    
    return lineBuffer.size() * 1.4; //1.4 is the estimative coefficient (it was arbitrary chosen)
}

vector<string> CSV::splitRow(string rawRow){
    vector<string> split;

    string currentValue = "";
    size_t rowSize = rawRow.size();
    for (int i = 0; i < rowSize; ++i){
        if (rawRow[i] == ',' || i == rowSize - 1){
            i < rowSize - 1 ? split.push_back(currentValue) : split.push_back(currentValue + rawRow[i]);

            currentValue = "";
        }
        else currentValue += rawRow[i];
    }

    return split;
}

CSV::CSVInfos CSV::readChunk(const char* filename, unsigned int dataRowsCount){
    using namespace CSV;

    CSVInfos infos;

    //Getting fields' names
    std::string fieldNames;
    IOHelper::file_getLine(filename, 0, &fieldNames);

    infos.fields = splitRow(fieldNames);

    //Getting data
    int rowsRead = 0;
    size_t startByteIdx = fieldNames.size() + 1; //Skip first line

    char* buffer = (char*) malloc(sizeof(char) * READ_CHUNK);
    size_t read_bytes = 1;

    string rawRow = "";
    while (rowsRead < dataRowsCount && read_bytes > 0){
        IOHelper::file_read(filename, READ_CHUNK, buffer, &read_bytes, startByteIdx);

        for (int i = 0; i < read_bytes; ++i){
            if (rowsRead == dataRowsCount) break;

            if (buffer[i] == '\n'){
                vector<string> sRow = splitRow(rawRow);
                infos.data.push_back(sRow);
              
                ++rowsRead;
                rawRow = "";
            }
            else rawRow += buffer[i];
        }
        startByteIdx += read_bytes;
    }

    return infos;
}