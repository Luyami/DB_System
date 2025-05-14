#include <iostream>
#include <fstream>

#include "db.h"
#include "csvhelper.h"

int main(){
    using namespace DB;
    using namespace CSV;

    Table t = Table::open("teste");

    {
        t.startBuildingSchema();

            t.addField("name", 30);
            t.addField("age", 2);

        t.stopBuildingSchema();
    }

    t.build_index("age", Index::Type::BPTree, Index::BuildingParams{50,32});

    return 0;
}