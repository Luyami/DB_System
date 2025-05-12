#include <stdexcept>
#include <string>
#include <chrono>

#include "iohelper.h"
#include "db.h"

#include <iostream>
#include <numeric>

#define PAGE_SIZE 256 //bytes
#define PAGE_HEADERS_MAX_SIZE 10 //bytes
#define PAGE_DATA_SIZE PAGE_SIZE - PAGE_HEADERS_MAX_SIZE //bytes
#define PAGE_MAX_FIELDS PAGE_HEADERS_MAX_SIZE - 1

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

void DBFile::startReading(int bytes, int startIdx){
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

void DBFile::startReading(int bytes){
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

//Class Table
bool Table::__isSchemaFinished(){
    {
    schema_file.startReading(schema_file.size());

        if (schema_file.getReadBuffer()[schema_file.getReadBytes()] == '\0') return true;

    schema_file.stopReading();
    }
    
    return false;
}

std::vector<int> Table::__getSchemaSizes(){
    std::vector<int> sizes;

    {
    schema_file.startReading(schema_file.size());

        char* buffer = schema_file.getReadBuffer();
        int bytes = schema_file.getReadBytes();

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
        int bytes = schema_file.getReadBytes();

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
        int bytes = schema_file.getReadBytes();

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

Table::SchemaInfos Table::__load_schemaInfos(){
    SchemaInfos s;
    
    s.finished     = __isSchemaFinished();
    s.names        = __getSchemaNames();
    s.sizes        = __getSchemaSizes();
    s.totalSize    = std::accumulate(s.sizes.begin(), s.sizes.end(), 0);
    s.field_count  = __getSchemaFieldCount();

    return s;
}

Table Table::open(const char* tableName){
    //Setting table
    Table t;

    std::string schema_fileName = std::string(tableName) + ".sch";
    std::string records_fileName = std::string(tableName) + ".rcs";

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

Table::SchemaInfos Table::getSchemaInfos() {return schemaInfos;}

bool Table::fieldExists(const char* fieldName){
    long long schemaSize = schema_file.size();

    {
    schema_file.startReading(schemaSize);
        char* buffer = schema_file.getReadBuffer();
        int bytes = schema_file.getReadBytes();

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
    if (schemaInfos.totalSize + size_in_bytes > PAGE_DATA_SIZE) return;
    if (schemaInfos.field_count + 1 > PAGE_MAX_FIELDS) return;

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
    schema_file.write("\0", 1); //Null terminator indicates that the schema is finished!
}

void Table::insertRecord(std::vector<std::string> data){
    //Record's file is formatted as following:
    //<field_count><first field byte offset>...<nth field byte offset><first field data>...<nth field data><padding>
    //field's byte offset starts counting after header info. so byte 0 = first byte of field data

    if (!schemaInfos.finished) return;
    if (data.size() != schemaInfos.field_count) return;

    //Checking if data not exceed bytes according to schema
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
    for (int i = 0; i < PAGE_SIZE - noPaddingBytes; ++i){
        data_string += (char) 3;
    }

    records_file.write(data_string.c_str(), data_string.size());
}

Table::Record Table::getRecordById(int id){
    std::vector<std::string> data;

    if (!schemaInfos.finished) return Record();
    if (records_file.size()/256 < id) return Record();
    if (id < 1) return Record();

    DBFile rf = records_file;
    {
        rf.startReading(256, PAGE_SIZE * (id - 1));

            char* buffer = rf.getReadBuffer();
            int field_count = (int) buffer[0];
            int firstDataByteIdx = field_count + 1;
   
            std::string field_value = "";
            for (int i = 0; i < field_count; ++i){
                int offset = (int) buffer[i + 1];
                int nextOffset = i < field_count - 1 ? (int) buffer[i + 2] : PAGE_SIZE - 1;
                
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

    return Record {schemaInfos.names, data};
}

//Struct Table::Record
void Table::Record::print(){
    for (int i = 0; i < fields.size(); ++i){
        std::cout << fields[i] << ": " << data[i] << '\n'; 
    }
}