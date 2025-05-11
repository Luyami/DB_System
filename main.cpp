#include <iostream>
#include <fstream>

#include "db.h"

int main(){
    using namespace DB;

    Table t = Table::open("wine");
    Table::SchemaInfos s = t.getSchemaInfos();

    std::vector<std::string> record = t.getRecordById(2);

    for (int i = 0; i < record.size(); ++i){
        std::cout << s.names[i] << ": " << record[i] << '\n';
    }

    return 0;
}