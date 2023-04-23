#include <cstdlib>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <chrono>

#include "leveldb/db.h"
#include "json/json.h"
#include "library.h"
#include "leveldb/comparator.h"

using namespace std;
using namespace std::chrono;

class RecordComparator : public leveldb::Comparator{
public:
    Schema* schema;
    explicit RecordComparator(unique_ptr<Schema>& s){
        this->schema = s.get();
    }
    int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const {
        string s1(a.ToString());
        string s2(b.ToString());
        char* _s1 = new char[s1.length() + 1];
        char* t_s1 = _s1;
        char* _s2 = new char[s2.length() + 1];
        char* t_s2 = _s2;
        strcpy (_s1, s1.c_str());
        strcpy (_s2, s2.c_str());
        for (int i = 0; i < this->schema->sort_attrs.size(); ++i) {
            int len = this->schema->attrs[i]->length;
            char *t1 = new char[len + 1];
            char* tt1 = t1;
            char *t2 = new char[len + 1];
            char* tt2 = t2;
            memcpy(t1, _s1, len);
            memcpy(t2, _s2, len);
            _s1 += len;
            _s2 += len;
//            cout << t1 << " " << t2 << endl;
            int res = strcmp(t1, t2);
            if(res == 0){
                delete[] tt1;
                delete[] tt2;
                continue;
            }
            else{
                delete[] tt1;
                delete[] tt2;
                delete[] t_s1;
                delete[] t_s2;
                return res > 0 ? 1 : -1;
            };
        }
        while(*_s1 != '\0' || *_s2 != '\0'){
            if(*_s1 == '\0' && *_s2 != '\0'){
                delete[] t_s1;
                delete[] t_s2;
                return -1;
            }
            if(*_s1 != '\0' && *_s2 == '\0'){
                delete[] t_s1;
                delete[] t_s2;
                return 1;
            }
            if(*_s1 < *_s2){
                delete[] t_s1;
                delete[] t_s2;
                return -1;
            }
            else{
                delete[] t_s1;
                delete[] t_s2;
                return 1;
            }
        }
        delete[] t_s1;
        delete[] t_s2;
        return 1;
    }
    // Ignore the following methods for now:
    const char* Name() const { return "RecordComparator"; }
    void FindShortestSeparator(std::string*, const leveldb::Slice&) const {}
    void FindShortSuccessor(std::string*) const {}
};

int main(int argc, const char* argv[]) {

    auto startTime = system_clock::now();
	if (argc < 4) {
		cout << "ERROR: invalid input parameters!" << endl;
		cout << "Please enter <schema_file> <input_file> <out_index> <sort_attributes>" << endl;
		exit(1);
	}

	// Do work here
    std::string schema_file(argv[1]);
    std::string input_file(argv[2]);
    std::string output_file(argv[3]);

    // Init schema
    // Parse the schema JSON file
    Json::Value schema;
    Json::Reader json_reader;
    std::ifstream schema_file_istream(schema_file, std::ifstream::binary);
    bool successful = json_reader.parse(schema_file_istream, schema, false);
    std::unique_ptr<Schema> s(new Schema);
    std::unordered_map<std::string, int> map;   // check sort attr exist
    for (int i = 0; i < schema.size(); ++i) {
        std::string name = schema[i].get("name", "UTF-8").asString();
        int length = schema[i].get("length", "UTF-8").asInt();
        std::string type = schema[i].get("type", "UTF-8").asString();
        s->attrs.push_back(std::make_unique<Attribute>(name, length, type));
        map[name] = i;
    }

    // Add sort_attributes
    for (int i = 4; i < argc; ++i) {
        std::string sorted_attr(argv[i]);
        if (map.find(sorted_attr) != map.end()) {
            s->sort_attrs.push_back(map[sorted_attr]);
        } else {
            std::cerr << "sorted attribute not exist" << std::endl;
        }
    }

//    printSchema(s);

    RecordComparator cmp(s);
    leveldb::DB *db;
    leveldb::Options options;
    options.create_if_missing = true;
    options.comparator = &cmp;
    leveldb::Status status = leveldb::DB::Open(options, "./leveldb_dir", &db);

    ifstream in_fp(input_file);
    string v;
    long unique_counter = 0;
    while(getline(in_fp, v)){
        stringstream ss(v);
        vector<string> data;
        string temp;
        while(getline(ss, temp, ',')){
            data.push_back(temp);
        }
        string key;
        for(auto i : s->sort_attrs){
            key += data[i];
        }
        key += to_string(unique_counter);
        db->Put(leveldb::WriteOptions(), leveldb::Slice(key), leveldb::Slice(v));
        unique_counter++;

    }
    std::ofstream out_fp(output_file, std::ios::out);
    // Iterator
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        leveldb::Slice value = it->value();
        std::string val_str = value.ToString();
        out_fp << val_str << endl;
    }
    assert(it->status().ok());  // Check for any errors found during the scan
    delete it;
    auto endTime = system_clock::now();
    auto duration = duration_cast<microseconds>(endTime - startTime);
    std::cout << double(duration.count()) * microseconds::period::num / microseconds::period::den << std::endl;
	return 0;
}