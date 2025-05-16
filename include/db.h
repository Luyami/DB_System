#ifndef DB_H
#define DB_H

#include <string>
#include <vector>

namespace DB{
    class DBFile;
    class Index;
    class BPTreeIDX;
    class Table;

    struct Record{
        std::vector<std::string> fields = {""};
        std::vector<std::string> data = {""};
        uint32_t rid;

        void print();
    };

    struct SchemaInfos{
        bool finished = false;
        std::vector<std::string> names = {""}; //fields' names
        std::vector<int> sizes = {-1}; //fields' size
        int totalSize = -1; //sum of all fields' size
        int field_count = -1;
    };


    //Interface to access DB files
    class DBFile{
        private:
            std::string path;
            
            //read temporary info
            bool isReading = false; 
            char* read_buffer = nullptr; //Stores read data
            size_t read_bytes = -1;  //Stores how many bytes were read in the last read operation

        public:
            static DBFile open(const char* path);
            static bool exists(const char* path);

            long long size();

            void write(const char* data);
            void write(const char* data, long long count);
            void write(const char* data, long long count, long long startIdx);
            
            //Reading pipeline
            void startReading(size_t bytes);
            void startReading(size_t bytes, size_t startIdx);

            char* getReadBuffer();
            int getReadBytes();

            void stopReading(); //Must be called for every startReading() call

        friend class Table;
    };

    class Index{
        public:
            enum Type{
                BPTree,
            };

            struct BuildingParams{
                uint8_t a0; //Min fill factor for b+tree/...
                uint32_t a1; //Max children for b+tree node/...
                uint32_t a2; //... 
                uint32_t a3; //...
            };

        protected:
            std::string name;
            Index::Type type;

            Table* referencingTable;
            std::string searchKey;
            int searchKeyPos;

            int idxInSpecificIndexArray;

            DBFile index_file;

        public:
            std::string getName();
            Index::Type getType();
            Table* getReferencingTable();
            std::string getSearchKey();
            DBFile getIndexFile();
            int getSearchKeyPos();

        friend class Table;
    };

    class BPTreeIDX: public Index{
        private:
            int max_children;
            int fill_factor;
            size_t node_size; //in bytes

            std::string __buildNodeString();
            size_t __getNodeStartReadingByte(uint32_t id);
            void __insertRecursively(uint32_t searchKey, uint32_t nodeId, uint32_t leftChild, uint32_t rightChild); //This is for internal nodes!
        public:
            BPTreeIDX(Index base, int max_children, int fill_factor);

            uint32_t findAppropriateLeaf(uint32_t search_key);
            uint32_t insertEntry(Record r);

            int getHeight();

            std::vector<Record> getRecords(int searchKey);

            void printNodeInfo(uint32_t id);
    };

    class Table{
        private:
            std::string name;

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
            DBFile schema_file;
            DBFile records_file;

            //index informations
            std::vector<Index> indices;
            std::vector<BPTreeIDX> indices_bp;

            static Table open(const char* tableName);

            DBFile getSchema();
            DBFile getRecords();

            SchemaInfos getSchemaInfos();

            bool fieldExists(const char* fieldName);

            //Schema building pipeline
            void startBuildingSchema();

            void addField(const char* fieldName, const int size_in_bytes);

            void stopBuildingSchema();

            //records building
            void insertRecord(std::vector<std::string> data); //Must follow schema order
            Record getRecordById(uint32_t id);
            std::vector<Record> getRecordsBy(const char* field, int searchKey);

            //index stuff
            void build_index(const char* searchKey, Index::Type idxType, Index::BuildingParams params);
    };
};

#endif