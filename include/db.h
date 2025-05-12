#ifndef DB_H
#define DB_H

#include <string>
#include <vector>

namespace DB{
    //Interface to access DB files
    class DBFile{
        private:
            std::string path;
            
            //read temporary info
            bool isReading = false; 
            char* read_buffer = nullptr; //Stores read data
            int read_bytes = -1;  //Stores how many bytes were read in the last read operation

        public:
            static DBFile open(const char* path);
            static bool exists(const char* path);

            long long size();

            void write(const char* data);
            void write(const char* data, long long count);
            
            //Reading pipeline
            void startReading(int bytes);
            void startReading(int bytes, int startIdx);

            char* getReadBuffer();
            int getReadBytes();

            void stopReading(); //Must be called for every startReading() call

        friend class Table;
    };

    class Table{
        public:
            struct SchemaInfos{
                bool finished = false;
                std::vector<std::string> names = {""}; //fields' names
                std::vector<int> sizes = {-1}; //fields' size
                int totalSize = -1; //sum of all fields' size
                int field_count = -1;
            };

            struct Record{
                std::vector<std::string> fields = {""};
                std::vector<std::string> data = {""};

                void print();
            };

        private:
            DBFile schema_file;
            DBFile records_file;

            //schema building temporary infos
            bool isBuildingSchema = false;
            int schema_size_in_bytes = -1;

            //this table's schema informations
            bool __isSchemaFinished();
            std::vector<std::string> __getSchemaNames();
            std::vector<int> __getSchemaSizes();
            int __getSchemaFieldCount();
            SchemaInfos __load_schemaInfos();
            SchemaInfos schemaInfos;
        public:
            static Table open(const char* tableName);

            DBFile getSchema();
            DBFile getRecords();

            Table::SchemaInfos getSchemaInfos();

            bool fieldExists(const char* fieldName);

            //Schema building pipeline
            void startBuildingSchema();

            void addField(const char* fieldName, const int size_in_bytes);

            void stopBuildingSchema();

            //records building
            void insertRecord(std::vector<std::string> data); //Must follow schema order
            Record getRecordById(int id);
    };
};

#endif