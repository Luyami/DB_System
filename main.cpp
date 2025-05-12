#include <iostream>
#include <fstream>

#include "db.h"
#include "csvhelper.h"

int main(){
    using namespace DB;
    using namespace CSV;

    CSVInfos infos = CSV::readChunk("datafiles/vinhos.csv", 50);
    std::cout << infos.fields[0] << " " << infos.fields [1] << " " << infos.fields[2] << " " << infos.fields[3] << '\n';
    
    for (int i = 0; i < 50; ++i){
        std::vector<std::string> d = infos.data[i];

        std::cout << d[0] << " " << d[1] << " " << d[2] << " " << d[3] << '\n';
    }

    return 0;
}