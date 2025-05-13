#include <iostream>
#include <fstream>

#include "db.h"
#include "csvhelper.h"

int main(){
    using namespace DB;
    using namespace CSV;

    Table t = Table::open("carro");

    {
        std::cout << "b";
        t.startBuildingSchema();
        std::cout << "e";

        t.addField("marca", 20);
        std::cout << "f";
        t.addField("ano", 4);

        std::cout << "g";
        t.stopBuildingSchema();
        std::cout << "h";
    }

    return 0;
}