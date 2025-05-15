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

    t.insertRecord({"luiz","37"});
    t.insertRecord({"joelio", "20"});
    t.insertRecord({"caba", "2"});
    t.build_index("age", Index::Type::BPTree, Index::BuildingParams{50,32});
    BPTreeIDX bi = t.indices_bp[0];
    bi.insertEntry(t.getRecordById(3));
    bi.insertEntry(t.getRecordById(1));
    bi.insertEntry(t.getRecordById(2));
    bi.printNodeInfo(1);

    return 0;
}