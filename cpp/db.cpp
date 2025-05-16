#include <stdexcept>
#include <string>
#include <algorithm>
#include <cstring>
#include <utility>
#include <cmath>
#include <chrono>

#include "iohelper.h"
#include "db.h"

#include <iostream>
#include <numeric>

#define DB_FILES_DIR "dbfiles";
#define DB_FILE_SCHEMA_EXTENSION ".sch";
#define DB_FILE_RECORDS_EXTENSION ".rcs";
#define DB_FILE_INDEX_EXTENSION ".idx";

#define RECORDS_PAGE_SIZE 256 //bytes
#define RECORDS_PAGE_HEADERS_MAX_SIZE 10 //bytes
#define RECORDS_PAGE_DATA_SIZE RECORDS_PAGE_SIZE - RECORDS_PAGE_HEADERS_MAX_SIZE //bytes
#define RECORDS_PAGE_MAX_FIELDS RECORDS_PAGE_HEADERS_MAX_SIZE - 1
#define RECORDS_PER_PAGE 1

using namespace DB;
using namespace IOHelper;

//Class DBFile
DBFile DBFile::open(const char* path){
    DBFile db;

    if (!exists(path)){
        file_create(path);
    }

    db.path = path;
    db.read_buffer = nullptr;
    db.read_bytes = -1;

    return db;
}

bool DBFile::exists(const char* path){
    return file_exists(path);
}

long long DBFile::size(){
    return file_size(path.c_str());
}

void DBFile::write(const char* data){
    bool success = file_write(path.c_str(), data);

    if (!success){
        throw std::runtime_error("Could not write to DB file!");
    }
}

void DBFile::write(const char* data, long long count){
    bool success = file_write(path.c_str(), data, count);

    if (!success){
        throw std::runtime_error("Could not write to DB file!");
    }
}

void DBFile::write(const char* data, long long count, long long startIdx){
    bool success = file_write(path.c_str(), data, count, startIdx);

    if (!success){
        throw std::runtime_error("Could not write to DB file!");
    }
}

void DBFile::startReading(size_t bytes, size_t startIdx){
    if (isReading) return;

    read_buffer = (char*) malloc(sizeof(char) * bytes);

    bool success = file_read(path.c_str(), bytes, read_buffer, &read_bytes, startIdx);
    isReading = true;

    if (!success){
        isReading = false;

        free(read_buffer);
        read_bytes = -1;

        throw std::runtime_error("Could not read DB file!");
    }
}

void DBFile::startReading(size_t bytes){
    startReading(bytes, 0);
}

char* DBFile::getReadBuffer(){
    if (isReading) return read_buffer;
    else return nullptr;
}

int DBFile::getReadBytes(){
    if (isReading) return read_bytes;
    else return -1;
}

void DBFile::stopReading(){
    if (!isReading) return;

    isReading = false;
    free(read_buffer);
    read_bytes = -1;
}

//Class Index
std::string  Index::getName()                {return name;}
Index::Type  Index::getType()                {return type;}
std::string  Index::getReferencingTable()    {return referencingTable;}
std::string  Index::getSearchKey()           {return searchKey;}
DBFile       Index::getIndexFile()           {return index_file;}
int          Index::getSearchKeyPos()        {return searchKeyPos;}

//Class BPTreeIDX
BPTreeIDX::BPTreeIDX(Index base, int max_children, int fill_factor){
    this->name             = base.getName();
    this->type             = base.getType();
    this->referencingTable = base.getReferencingTable();
    this->searchKey        = base.getSearchKey();
    this->searchKeyPos     = base.getSearchKeyPos();
    this->index_file       = base.getIndexFile();

    this->max_children = max_children;
    this->fill_factor  = fill_factor;

    this->node_size = 8*(max_children-1) + 4*3 + 1;
}

//Internal Node is formatted as:
//<is_leaf:1B><id_parent:4B><id_1child:4B><searchkey1:4B>...<id_nchild:4B><searchkeyn:4B><id_n+1child:4B>
//Size is 8 * max_children + 1
//Leaf Node is formatted as:
//<is_leaf:1B><id_parent:4B><lf_id:4B><rf_id:4B><tuple1:8B><tuple2:8B>...<tuplen:8B><padding>
//tuplen = skn:r_idn (without the :)
//internal node and life node has fixed size of leaf node size because of padding
std::string BPTreeIDX::__buildNodeString(){
    static constexpr char FILL_CHAR = (char) 0;

    std::string s(8*(max_children-1) + 4*3 + 1, FILL_CHAR);
    s[0] = 0;

    return s;
}

size_t BPTreeIDX::__getNodeStartReadingByte(uint32_t id){
    return 7 + (id - 1) * node_size;
}

uint32_t BPTreeIDX::findAppropriateLeaf(uint32_t search_key){
    uint32_t currentNodeId = 1; //1 is always for root
    bool isNodeLeaf = false;

    while (!isNodeLeaf){
        {  
            index_file.startReading(node_size, __getNodeStartReadingByte(currentNodeId));

                char* buffer = index_file.getReadBuffer();
                size_t bytes = index_file.getReadBytes();

                if (buffer[0] == 1){ //is leaf
                    isNodeLeaf = true;
                    break;
                }

                //if it isn't leaf, lets go down!!!!
                uint32_t currentSk;
                int currentSkPos = 0;

                while (true){ //Runs at most max_children - 1 times
                    char* readingPointer = buffer + 9 + currentSkPos * 4;
                    std::memcpy(&currentSk, readingPointer, sizeof(uint32_t));

                    if (currentSkPos == max_children - 1){ //Search key is bigger than all the other ones!
                        std::memcpy(&currentNodeId, readingPointer - 12, sizeof(uint32_t));
                        break;
                    }

                    if (search_key < currentSk){ //Jump to a child node!
                        std::memcpy(&currentNodeId, readingPointer - 4, sizeof(uint32_t));
                        break;
                    }

                    ++currentSkPos;
                }

            index_file.stopReading();
        }
    }

    return currentNodeId;
}

void BPTreeIDX::__insertRecursively(uint32_t searchKey, uint32_t nodeId, uint32_t left_child, uint32_t right_child){
    using namespace std;

    static int count = 0;
    ++count;
    std::cout << "e" << '\n';
    if (count > 4) return;

    size_t nodeFirstByte = __getNodeStartReadingByte(nodeId);
    
    {
        index_file.startReading(node_size, nodeFirstByte);

            char* buffer = index_file.getReadBuffer();
            int bytes = index_file.getReadBytes();

            vector<pair<uint32_t, uint32_t>> pointersANDsks;
            uint32_t lastPointer;
            int pairCount = 0;
            while(true){
                uint32_t maybeEmptySk;
                std::memcpy(&maybeEmptySk, buffer + 5 + pairCount * 8 + 4, sizeof(uint32_t));

                if (pointersANDsks.size() == max_children - 1){ //Node is full
                    std::cout << searchKey << " " << node_size << " bytes" << '\n';
                    std::memcpy(&lastPointer, buffer + 5 + (max_children - 2) * 8, sizeof(uint32_t));

                    //lets build the theoretical node with max_children + 1 children
                    int newSearchKeyPos = 0;
                    for (int i = 0; i < pointersANDsks.size(); ++i){
                        if (pointersANDsks[i].first > searchKey) break;
                        else ++newSearchKeyPos;
                    }

                    //shifting elements starting from newSearchKeyPos
                    pointersANDsks.resize(pointersANDsks.size() + 1);

                    for (int i = pointersANDsks.size() - 2; i >= newSearchKeyPos; --i){
                        pointersANDsks[i + 1] = pointersANDsks[i];
                    }

                    pointersANDsks[newSearchKeyPos] = pair<uint32_t, uint32_t>(searchKey, left_child);
                    if (newSearchKeyPos < pointersANDsks.size() - 1){
                        pointersANDsks[newSearchKeyPos + 1].second = right_child;
                    }
                    else{
                        lastPointer = right_child;
                    }
                    //;;

                    std::string firstNode = __buildNodeString(); //this will be the node with nodeId id
                    std::string secondNode = __buildNodeString(); //this will be the node with (file_size - 7)/node_size id;
                    std::string newRoot = "";

                    uint32_t firstNodeId = nodeId;
                    uint32_t secondNodeId = (index_file.size() - 7)/node_size;
                    uint32_t rootId = secondNodeId + 1;
                    uint32_t parentId;
                    std::memcpy(&parentId, buffer + 1, sizeof(uint32_t));

                    if (parentId == 0){ //It's a root node! We have to create a new root. its id will be secondNodeId + 1
                        newRoot = __buildNodeString();
                        newRoot[0] = 0;
                    }

                    int median_index = pointersANDsks.size()/2;

                    firstNode[0] = 0;
                    parentId = parentId == 0 ? rootId : parentId;
                    std::memcpy((char*) firstNode.data() + 1, &parentId, sizeof(uint32_t));
                    for (int i = 0; i < median_index; ++i){
                        std::memcpy((char*) firstNode.data() + 5 + i * 8, &pointersANDsks[i].second, sizeof(uint32_t)); //child id
                        std::memcpy((char*) firstNode.data() + 9 + i * 8, &pointersANDsks[i].first, sizeof(uint32_t)); //sk
                    }
                    std::memcpy((char*) firstNode.data() + 5 + median_index * 8, &pointersANDsks[median_index].second, sizeof(uint32_t));

                    secondNode[0] = 0;
                    std::memcpy((char*) secondNode.data() + 1, &parentId, sizeof(uint32_t));
                    for (int i = median_index + 1; i < pointersANDsks.size(); ++i){
                        std::memcpy((char*) secondNode.data() + 5 + (i - median_index + 1) * 8, &pointersANDsks[i].second, sizeof(uint32_t)); //child id
                        std::memcpy((char*) secondNode.data() + 9 + (i - median_index + 1) * 8, &pointersANDsks[i].first, sizeof(uint32_t)); //sk
                    }
                    std::memcpy((char*) firstNode.data() + 5 + (max_children - 2) * 8, &lastPointer, sizeof(uint32_t));

                    std::cout << "saving" << '\n';
                    index_file.write(firstNode.data(), node_size, __getNodeStartReadingByte(firstNodeId));
                    index_file.write(secondNode.data(), node_size);
                    if (!newRoot.empty()) index_file.write(newRoot.data(), node_size);
                    std::cout << "saved" << '\n';
                    index_file.stopReading();
                    __insertRecursively(pointersANDsks[median_index].second, parentId, firstNodeId, secondNodeId);

                    break;
                }
                else if(maybeEmptySk == 0){ //empty slot
                    int newSearchKeyPos = 0;
                    for (int i = 0; i < pointersANDsks.size(); ++i){
                        if (pointersANDsks[i].first > searchKey) break;
                        else ++newSearchKeyPos;
                    }

                    //we have to shift all entries starting from newSearchKeyPos
                    std::memcpy(buffer + 5 + (pointersANDsks.size() + 1) * 8, buffer + 5 + pointersANDsks.size() * 8, sizeof(uint32_t)); //moving last pointer
                    for (int i = pointersANDsks.size() - 1; i >= newSearchKeyPos; --i){
                        int entryStartByte = 5 + i*8;
                        int shiftStartyByte = entryStartByte + 8;

                        std::memcpy(buffer + shiftStartyByte, buffer + entryStartByte, sizeof(uint32_t)); //copying child id
                        std::memcpy(buffer + shiftStartyByte + 4, buffer + entryStartByte + 4, sizeof(uint32_t)); //copying sk
                    }

                    //now we can set our new search key
                    std::memcpy(buffer + 5 + newSearchKeyPos*8 + 4, &searchKey, sizeof(uint32_t)); //setting sk value
                    if (pointersANDsks.size() > 0){
                        std::memcpy(buffer + 5 + newSearchKeyPos*8, buffer + 5 + newSearchKeyPos*8 + 8, sizeof(uint32_t)); //setting left child
                    } 
                    else{
                        std::memcpy(buffer + 5 + newSearchKeyPos*8, &left_child, sizeof(uint32_t)); //setting left child
                    }
                    std::memcpy(buffer + 5 + newSearchKeyPos*8 + 8, &right_child, sizeof(uint32_t)); //setting right child
                    std::cout << "wiritng" << '\n';
                    index_file.write(buffer, node_size, nodeFirstByte);

                    break;
                }
                else{ //there is a pair here
                    uint32_t cPointer;
                    uint32_t sk;

                    std::memcpy(&cPointer, buffer + 5 + pairCount * 8, sizeof(uint32_t));
                    std::memcpy(&sk, buffer + 5 + pairCount * 8 + 4, sizeof(uint32_t));

                    std::cout << searchKey << " " << cPointer << '\n';

                    pointersANDsks.push_back(pair<uint32_t, uint32_t>(sk, cPointer));
                }

                ++pairCount;
            }

        index_file.stopReading();
    }
}

uint32_t BPTreeIDX::insertEntry(Record r){
    //For now, search keys can only be non-negative and non-zero numerical values!

    size_t file_size = index_file.size();
    size_t newId = (file_size - 7)/node_size + 1;
    
    std::string node;
    uint32_t sk = std::stoi(r.data[searchKeyPos]) + 1;

    if (file_size - 7 == 0){ //Doesn't have any node! Lets build a leaf-root
        node = __buildNodeString();

        node[0] = 1; //is leaf!

        std::memcpy((char*) node.data() + 13, &sk, sizeof(uint32_t)); //Copying search key
        std::memcpy((char*) node.data() + 17, &r.rid, sizeof(uint32_t)); //Copying r_id

        index_file.write(node.c_str(), node.size());
    }
    else{ //We have a root!!!
        uint32_t leaf_id = findAppropriateLeaf(sk);
        size_t node_firstByte = __getNodeStartReadingByte(leaf_id);

        {
            index_file.startReading(node_size, node_firstByte);

                char* buffer = index_file.getReadBuffer();
                size_t bytes = index_file.getReadBytes();

                //First, lets check whether leaf is full or not
                int tupleCount = 0;
                bool isLeafFull = false;
                std::vector<std::pair<uint32_t, uint32_t>> rids;
                while(true){
                    if (rids.size() == max_children - 1){ //Leaf is full
                        isLeafFull = true;

                        break;
                    }

                    uint32_t maybeEmpty;
                    std::memcpy(&maybeEmpty, buffer + 13 + 8 * tupleCount, sizeof(uint32_t));

                    if (maybeEmpty == 0){ //this slot is empty!
                        rids.push_back(std::pair<uint32_t, uint32_t>(sk, r.rid));

                        //Lets sort the search keys!
                        std::sort(rids.begin(), rids.end());

                        //Now lets write the new sorted leaf
                        for (int i = 0; i <= tupleCount; ++i){
                            std::memcpy(buffer + 13 + 8 * i, &rids[i].first, sizeof(uint32_t)); //Copying search key
                            std::memcpy(buffer + 13 + 8 * i + 4, &rids[i].second, sizeof(uint32_t)); //Copying r_id
                        }
                        
                        index_file.write(buffer, bytes, node_firstByte);

                        break;
                    }
                    else{ //There is something in slot
                        uint32_t thisSK;
                        uint32_t thisRID;

                        std::memcpy(&thisSK, buffer + 13 + 8 * tupleCount, sizeof(uint32_t));
                        std::memcpy(&thisRID, buffer + 13 + 8 * tupleCount + 4, sizeof(uint32_t));

                        rids.push_back(std::pair<uint32_t, uint32_t>(thisSK, thisRID));
                    }

                    ++tupleCount;
                }

                if (isLeafFull){ //Leaf is full! lets split
                    rids.push_back(std::pair<uint32_t, uint32_t>(sk, r.rid));
                    std::sort(rids.begin(), rids.end());

                    int keys_in_firstLeafNode = max_children/2;
                    int keys_in_secondLeafNode = max_children - keys_in_firstLeafNode;
                    
                    //the first leaf node will be the node with id leaf_id (which is in buffer already)
                    //the second leaf node will be the node with id newId

                    std::string firstLN = __buildNodeString();
                    std::string secondLN = __buildNodeString();

                    //first leaf node
                    firstLN[0] = 1;
                    std::memcpy((char*) firstLN.data() + 1, buffer + 1, sizeof(uint32_t));
                    for (int i = 0; i < keys_in_firstLeafNode; ++i){
                        std::memcpy((char*) firstLN.data() + 13 + 8 * i, &rids[i].first, sizeof(uint32_t)); //Copying search key
                        std::memcpy((char*) firstLN.data() + 13 + 8 * i + 4, &rids[i].second, sizeof(uint32_t)); //Copying r_id
                    }

                    //second leaf node
                    secondLN[0] = 1;
                    std::memcpy((char*) secondLN.data() + 1, buffer + 1, sizeof(uint32_t));
                    for (int i = 0; i < keys_in_secondLeafNode; ++i){
                        std::memcpy((char*) secondLN.data() + 13 + 8 * i, &rids[keys_in_firstLeafNode + i].first, sizeof(uint32_t)); //Copying search key
                        std::memcpy((char*) secondLN.data() + 13 + 8 * i + 4, &rids[keys_in_firstLeafNode + i].second, sizeof(uint32_t)); //Copying r_id
                    }

                    uint32_t parent_id;
                    std::memcpy(&parent_id, buffer + 1, sizeof(uint32_t));
                    uint32_t median_sk = rids[keys_in_firstLeafNode].first;
                    if (parent_id == 0){ //Doesn't have a parent
                        parent_id = newId + 1;
                        std::cout << "desont hvav arnet" << '\n';
                        std::string newParent = __buildNodeString();
                        newParent[0] = 0;
                        std::memcpy((char*) newParent.data() + 5, &leaf_id, sizeof(uint32_t)); //left child
                        std::memcpy((char*) newParent.data() + 9, &median_sk, sizeof(uint32_t)); //sk
                        std::memcpy((char*) newParent.data() + 13, &newId, sizeof(uint32_t)); //right child

                        std::memcpy((char*) firstLN.data() + 1, &parent_id, sizeof(uint32_t));
                        std::memcpy((char*) secondLN.data() + 1, &parent_id, sizeof(uint32_t));

                        std::memcpy((char*) secondLN.data() + 5, &leaf_id, sizeof(uint32_t));
                        std::memcpy((char*) secondLN.data() + 9, buffer + 9, sizeof(uint32_t));

                        std::memcpy((char*) firstLN.data() + 9, &newId, sizeof(uint32_t));

                        index_file.write(firstLN.data(), node_size, __getNodeStartReadingByte(leaf_id)); //first node
                        index_file.write(secondLN.data(), node_size); //second node
                        index_file.write(newParent.data(), node_size); //parent node
                    }
                    else{
                        index_file.write(firstLN.data(), node_size, __getNodeStartReadingByte(leaf_id)); //first node
                        index_file.write(secondLN.data(), node_size); //second node

                        std::cout << median_sk << " " << parent_id << " " << leaf_id << " " << newId << '\n';
                        index_file.stopReading();
                        
                        __insertRecursively(median_sk, parent_id, leaf_id, newId);
                    }      
                }

            index_file.stopReading();
        }
    }

    return newId;
}

void BPTreeIDX::printNodeInfo(uint32_t id){
    {
        index_file.startReading(node_size, __getNodeStartReadingByte(id));

            char* buffer = index_file.getReadBuffer();
            int bytes = index_file.getReadBytes();
     
            int counter = 0;
            if (buffer[0] == 0){ //isn't leaf
                std::cout << "Leaf: false\n";
                for (int i = 1; i < bytes; ++i){
                    if (counter == 4){
                        std::cout << " ";
                        counter = 0;
                    }

                    std::cout << (int) buffer[i] << ".";

                    ++counter;
                }
            }
            else{ //is leaf
                std::cout << "Leaf: true\n";
                for (int i = 1; i < bytes; ++i){
                    if (counter == 4){
                        std::cout << " ";
                        counter = 0;
                    }

                    std::cout << (int) buffer[i] << ".";

                    ++counter;
                }
            }

        index_file.stopReading();
    }
}

//Class Table
bool Table::__isSchemaFinished(){
    {
    schema_file.startReading(schema_file.size());
     
        if (schema_file.getReadBuffer()[schema_file.getReadBytes() - 1] == (char) 3) return true;

    schema_file.stopReading();
    }

    return false;
}

std::vector<int> Table::__getSchemaSizes(){
    std::vector<int> sizes;

    {
    schema_file.startReading(schema_file.size());

        char* buffer = schema_file.getReadBuffer();
        size_t bytes = schema_file.getReadBytes();

        std::string currentNumber = "";
        bool shouldIgnoreChar = true; //According to our schema formatting, the file never starts with a field's size
        for (int i = 0; i < bytes; ++i){
            if (buffer[i] == ':'){
                shouldIgnoreChar = false;
            }
            else if (buffer[i] == ';'){
                shouldIgnoreChar = true;

                sizes.push_back(std::stoi(currentNumber));

                currentNumber = "";
            }
            else if (!shouldIgnoreChar){
                currentNumber += buffer[i];
            }
            
        }

    schema_file.stopReading();
    }

    return sizes;
}

int Table::__getSchemaFieldCount(){
    int fieldsCount = 0;

    {
    schema_file.startReading(schema_file.size());

        char* buffer = schema_file.getReadBuffer();
        size_t bytes = schema_file.getReadBytes();

        for (int i = 0; i < bytes; ++i){
            if (buffer[i] == ':') fieldsCount += 1; //For every field, we have a ':' or ';' so it's valid to calculate field's count like this 
        }

    schema_file.stopReading();
    }

    return fieldsCount;
}

std::vector<std::string> Table::__getSchemaNames(){
    std::vector<std::string> names;

    long long schemaSize = schema_file.size();
    {
    schema_file.startReading(schemaSize);
        char* buffer = schema_file.getReadBuffer();
        size_t bytes = schema_file.getReadBytes();

        std::string foundField = "";

        bool shouldIgnoreChar = false;
        for (int i = 0; i < bytes; ++i){
            if (buffer[i] == ':'){
                names.push_back(foundField);

                foundField = "";
                shouldIgnoreChar = true;
            }
            else if (buffer[i] == ';'){
                shouldIgnoreChar = false;
            }
            else if (!shouldIgnoreChar){
                foundField += buffer[i];
            }
        }
        
    schema_file.stopReading();
    }

    return names;
}

SchemaInfos Table::__load_schemaInfos(){
    SchemaInfos s;
    
    s.finished     = __isSchemaFinished();
    s.names        = __getSchemaNames();
    s.sizes        = __getSchemaSizes();
    s.totalSize    = std::accumulate(s.sizes.begin(), s.sizes.end(), 0);
    s.field_count  = __getSchemaFieldCount();

    return s;
}

Table Table::open(const char* tableName){
    std::string dir = DB_FILES_DIR;

    //Setting table
    Table t;
    t.name = tableName;

    std::string schema_fileName  =  dir + "/" + std::string(tableName) + DB_FILE_SCHEMA_EXTENSION;
    std::string records_fileName =  dir + "/" + std::string(tableName) + DB_FILE_RECORDS_EXTENSION;

    if (!DBFile::exists(schema_fileName.c_str())){
        file_create(schema_fileName.c_str());   
    }

    if (!DBFile::exists(records_fileName.c_str())){
        file_create(records_fileName.c_str());
    }
    
    t.schema_file.path = schema_fileName;
    t.schemaInfos = t.__load_schemaInfos();
    t.records_file.path = records_fileName;

    return t;
}

DBFile Table::getSchema() {return schema_file;}
DBFile Table::getRecords() {return records_file;}

SchemaInfos Table::getSchemaInfos() {return schemaInfos;}

bool Table::fieldExists(const char* fieldName){
    long long schemaSize = schema_file.size();

    {
    schema_file.startReading(schemaSize);
        char* buffer = schema_file.getReadBuffer();
        size_t bytes = schema_file.getReadBytes();

        std::string targetField = std::string(fieldName);
        std::string foundField = "";

        bool shouldIgnoreChar = false;
        for (int i = 0; i < bytes; ++i){
            if (buffer[i] == ':'){
                if (foundField == targetField) return true;

                foundField = "";
                shouldIgnoreChar = true;
            }
            else if (buffer[i] == ';'){
                shouldIgnoreChar = false;
            }
            else if (!shouldIgnoreChar){
                foundField += buffer[i];
            }
        }
        
    schema_file.stopReading();
    }

    return false;
}

void Table::startBuildingSchema(){
    if (isBuildingSchema) return;
    if (schemaInfos.finished) return;
    
    isBuildingSchema = true;
}

void Table::addField(const char* fieldName, const int size_in_bytes){
    if (!isBuildingSchema) return;

    //field name can't contain ':' nor ';'

    //Schema's file is formatted as following:
    //<field>:<field_size>;

    if (fieldExists(fieldName)) return;
    if (schemaInfos.totalSize + size_in_bytes > RECORDS_PAGE_DATA_SIZE) return;
    if (schemaInfos.field_count + 1 > RECORDS_PAGE_MAX_FIELDS) return;

    std::string data_string = std::string(fieldName) + ':' + std::to_string(size_in_bytes) + ';';
    schema_file.write(data_string.c_str(), data_string.size());

    schemaInfos.names.push_back(fieldName);
    schemaInfos.sizes.push_back(size_in_bytes);
    schemaInfos.field_count += 1;
    schemaInfos.totalSize += size_in_bytes;
}

void Table::stopBuildingSchema(){
    if (!isBuildingSchema) return;

    isBuildingSchema = false;
    schema_size_in_bytes = -1;
    schemaInfos.finished = true;
    schema_file.write("\x03", 1); //char 3 indicates that the schema is finished!
}

void Table::insertRecord(std::vector<std::string> data){
    //Record's file is formatted as following:
    //<field_count><first field byte offset>...<nth field byte offset><first field data>...<nth field data><padding>
    //field's byte offset starts counting after header info. so byte 0 = first byte of field data

    if (!schemaInfos.finished) return;
    if (data.size() != schemaInfos.field_count) return;

    //Checking if data doesn't exceed bytes according to schema
    for (int i = 0; i < data.size(); ++i){
        if (data[i].size() > schemaInfos.sizes[i]) return;
    }

    std::string data_string = "";
    data_string += (char) schemaInfos.field_count; //1 byte

    //Setting offset bytes
    int offsetCount = 0;
    for (int i = 0; i < data.size(); ++i){
        data_string += (char) offsetCount;

        offsetCount += data[i].size();
    }

    //Setting field data
    for (int i = 0; i < data.size(); ++i){
        data_string += data[i];
    }

    //Setting padding bytes
    int noPaddingBytes = data_string.size();
    for (int i = 0; i < RECORDS_PAGE_SIZE - noPaddingBytes; ++i){
        data_string += (char) 3;
    }

    records_file.write(data_string.c_str(), data_string.size());
}

Record Table::getRecordById(uint32_t id){
    std::vector<std::string> data;

    if (!schemaInfos.finished) return Record();
    if (records_file.size()/256 < id) return Record();
    if (id < 1) return Record();

    DBFile rf = records_file;
    {
        rf.startReading(256, RECORDS_PAGE_SIZE * (id - 1));

            char* buffer = rf.getReadBuffer();
            int field_count = (int) buffer[0];
            int firstDataByteIdx = field_count + 1;
   
            std::string field_value = "";
            for (int i = 0; i < field_count; ++i){
                int offset = (int) buffer[i + 1];
                int nextOffset = i < field_count - 1 ? (int) buffer[i + 2] : RECORDS_PAGE_SIZE - 1;
                
                for (int j = firstDataByteIdx + offset; j < firstDataByteIdx + nextOffset; ++j){
                    char c = buffer[j];
           
                    if (c == (char) 3) break; // (char) 3 is our end of record

                    field_value += c;
                }
       
                data.push_back(field_value);
                field_value = "";
            }

        rf.stopReading();
    }

    return Record {schemaInfos.names, data, id};
}

void Table::build_index(const char* searchKey, Index::Type idxType, Index::BuildingParams params){
    if (!Table::fieldExists(searchKey)){
        throw std::runtime_error("Can't use " + std::string(searchKey) + " as search key, because it isn't part of the schema!");
    }

    std::string dir = DB_FILES_DIR;

    int searchKeyPos; //This indicates what field is being used as search key in the index header
    searchKeyPos = std::distance(
        schemaInfos.names.begin(),
        std::find(schemaInfos.names.begin(), schemaInfos.names.end(), searchKey)
    );

    std::string idxName = this->name + std::to_string((int) idxType) + "_" + std::to_string(searchKeyPos);
    std::string idxPath = dir + "/" + idxName + DB_FILE_INDEX_EXTENSION;

    if (!DBFile::exists(idxPath.c_str())){
        file_create(idxPath.c_str());
    }

    if (file_size(idxPath.c_str()) > 0) return;

    //building header general informations
    std::string header = "";
    header += (char) idxType; //index type (1 byte)
    header += (char) searchKeyPos; //field being used (1 byte)
    std::cout << header.size() << '\n';

    Index idx;
    idx.name = idxName;
    idx.type = idxType;
    idx.referencingTable = this->name; 
    idx.searchKey = searchKey;
    idx.searchKeyPos = searchKeyPos;
    idx.index_file.path = idxPath;

    if (idxType == Index::Type::BPTree){
        header.resize(header.size() + sizeof(uint32_t));
        std::memcpy((char*) header.data() + header.size() - 4, &params.a1, sizeof(uint32_t)); //max children (4 bytes)

        header += (char) params.a0;

        idx.idxInSpecificIndexArray = indices_bp.size();
        BPTreeIDX bp_idx(idx, params.a1, params.a0);

        this->indices_bp.push_back(bp_idx);
    }
    //... one if clause for each type

    indices.push_back(idx);
    idx.index_file.write(header.c_str(), header.size());
}

//Struct Table::Record
void Record::print(){
    for (int i = 0; i < fields.size(); ++i){
        std::cout << fields[i] << ": " << data[i] << '\n'; 
    }
}