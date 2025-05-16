#include "csvhelper.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

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
    std::vector<std::string> lines;
    std::ifstream file(filename);

    std::string line;
    CSVInfos infos;

    //getting infos
    std::getline(file, line);
    infos.fields = splitRow(line);

    for (int i = 0; i < dataRowsCount && std::getline(file, line); ++i) {
        infos.data.push_back(splitRow(line));
    }

    file.close();

    return infos;
}