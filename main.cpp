#include <iostream>
#include <fstream>

#include "db.h"
#include "csvhelper.h"

int main(){
    using namespace DB;
    using namespace CSV;

    //Creating aux table
    Table wines = Table::open("vinhos");

    {
        wines.startBuildingSchema();

            wines.addField("id", 4);
            wines.addField("rotulo", 40);
            wines.addField("ano-colheita", 4);
            wines.addField("tipo", 8);

        wines.stopBuildingSchema();
    }
    wines.build_index("ano-colheita", DB::Index::Type::BPTree, DB::Index::BuildingParams{50, 10});
    CSVInfos wines_csv = readChunk("datafiles/vinhos.csv", 1000);
    for (std::vector<std::string> data: wines_csv.data){
        wines.insertRecord(data);
    }

    //
    Table trab = Table::open("trab");

    {
        trab.startBuildingSchema();

            trab.addField("id", 4);
            trab.addField("rotulo", 40);
            trab.addField("ano-colheita", 4);
            trab.addField("tipo", 6);

        trab.stopBuildingSchema();
    }

    std::ifstream infile("in.txt");
    std::string line;

    int max_children = -1;

    if (!std::getline(infile, line)) {
        std::cerr << "Arquivo vazio ou inválid!!!!!!!!!!.\n";
        return 1;
    }

    if (line.rfind("FLH/", 0) == 0) {
        max_children = std::stoi(line.substr(4));
    } else {
        std::cerr << "Linha FLH invalida.\n";
        return 1;
    }

    uint32_t m = max_children;

    trab.build_index("ano-colheita", DB::Index::Type::BPTree, DB::Index::BuildingParams{50, 10});

    DBFile out = DBFile::open("out.txt");

    while (std::getline(infile, line)) {
        if (line.rfind("INC:", 0) == 0) {
            int key = std::stoi(line.substr(4));

            std::vector<Record> rcs = wines.getRecordsBy("ano-colheita", key);
            
            for (Record r: rcs){
                trab.insertRecord(r.data);
            }
            std::string outData = "INC:" + std::to_string(key) + '/' + std::to_string(rcs.size()) + '\n';
            out.write(outData.data(), outData.size());

        } else if (line.rfind("BUS=:", 0) == 0) {
            int key = std::stoi(line.substr(5));
            
            std::vector<Record> rcs = trab.getRecordsBy("ano-colheita", key);

            for (Record r: rcs){
                r.print();
            }
            std::string outData = "BUS:=" + std::to_string(key) + '/' + std::to_string(rcs.size()) + '\n';
            out.write(outData.data(), outData.size());
        } else {
            std::cerr << "Comando invalido: " << line << "\n";
        }
    }

    std::string outData = "H" + '/' + std::to_string(trab.indices_bp[0].getHeight()) + '\n';
    out.write(outData.data(), outData.size());

    return 0;
}