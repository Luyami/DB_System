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
    t.insertRecord({"idk", "5"});
    t.insertRecord({"idk", "6"});
    t.insertRecord({"idk", "9"});
    
    t.insertRecord({"idk", "11"});
    t.insertRecord({"idk", "12"});
    t.insertRecord({"idk", "13"});
    t.insertRecord({"idk", "14"});
    t.insertRecord({"idk", "15"});
    t.insertRecord({"idk", "16"});
    t.insertRecord({"idk", "17"});
    t.build_index("age", Index::Type::BPTree, Index::BuildingParams{50,4});
    BPTreeIDX bi = t.indices_bp[0];
    bi.insertEntry(t.getRecordById(3));
    bi.insertEntry(t.getRecordById(1));
    bi.insertEntry(t.getRecordById(2));
    bi.insertEntry(t.getRecordById(4));
    bi.insertEntry(t.getRecordById(5));
    bi.insertEntry(t.getRecordById(6));
    bi.insertEntry(t.getRecordById(7));
    bi.insertEntry(t.getRecordById(8));
    bi.printNodeInfo(1);
    std::cout << '\n';
    bi.printNodeInfo(2);
    std::cout << '\n';
    bi.printNodeInfo(3);
    std::cout << '\n';
    bi.printNodeInfo(4);
    std::cout << '\n';
    bi.printNodeInfo(5);

    return 0;
}