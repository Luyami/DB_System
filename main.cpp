#include <iostream>
#include <fstream>

#include "db.h"
#include "csvhelper.h"

int main(){
    using namespace DB;
    using namespace CSV;

    DBFile d = DBFile::open("teste.ss");
    
    {
        d.startReading(d.size());

        char* buffer = d.getReadBuffer();
        int bytes = d.getReadBytes();

        for (int i = 0 ; i < bytes ; ++i){
            std::cout << buffer[i];
        }

        d.stopReading();
    }

    return 0;
}