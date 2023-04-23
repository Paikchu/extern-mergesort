#include <memory>
#include <chrono>
#include "library.h"
#include "json/json.h"
using namespace std::chrono;


int main(int argc, const char *argv[]) {
    auto startTime = system_clock::now();
    if (argc < 7) {
        std::cout << "ERROR: invalid input parameters!" << std::endl;
        std::cout << "Please enter <schema_file> <input_file> <output_file> <mem_capacity> <k> <sorting_attributes>"
                  << std::endl;
        exit(1);
    }
    std::string schema_file(argv[1]);
    std::string input_file(argv[2]);
    std::string output_file(argv[3]);
    int mem_capacity = atoi(argv[4]);
    int k = atoi(argv[5]);

    // Parse the schema JSON file
    Json::Value schema;
    Json::Reader json_reader;
    // Support for std::string argument is added in C++11,
    // so you don't have to use .c_str() if you are on that.
    std::ifstream schema_file_istream(schema_file, std::ifstream::binary);
    bool successful = json_reader.parse(schema_file_istream, schema, false);
    if (!successful) {
        std::cout << "ERROR: " << json_reader.getFormattedErrorMessages() << std::endl;
        exit(1);
    }

    // Do the sort
    // Your implementation
    int recordLength = 1;
    std::unique_ptr<Schema> s(new Schema);
    std::unordered_map<std::string, int> map;   // check sort attr exist
    for (int i = 0; i < schema.size(); ++i) {
        std::string name = schema[i].get("name", "UTF-8").asString();
        int length = schema[i].get("length", "UTF-8").asInt();
        std::string type = schema[i].get("type", "UTF-8").asString();
        s->attrs.push_back(std::make_unique<Attribute>(name, length, type));
        map[name] = i;
        recordLength += length + 1;
    }
//    std::cout << recordLength << std::endl;
    for (int i = 6; i < argc; ++i) {
        std::string sorted_attr(argv[i]);
        if (map.find(sorted_attr) != map.end()) {
            s->sort_attrs.push_back(map[sorted_attr]);
        } else {
            std::cerr << "sorted attribute not exist" << std::endl;
        }
    }

//    printSchema(s);

    std::vector<RunIterator*> iterators;
    std::ifstream in_fp(input_file, std::ios::in);
    in_fp.seekg(0, in_fp.end);
    long totalBytes = in_fp.tellg();
    long totalLines = totalBytes / recordLength;
    in_fp.seekg(0);
    in_fp.close();

    long bufSize = mem_capacity / (k);
    int output_buffer = 10000;  // buffer能存放的record的数量
    long startPos = 0;
    long runLength = bufSize;
    std::ofstream out_fp;
    int numOfPass = log(totalLines / bufSize) / log(k);
    for(int q = 0; q <= numOfPass; ++q){
        if(q == numOfPass){
            out_fp.open(output_file, std::ios::out);
        }
        else{
            out_fp.open("output_file" + std::to_string(q) + ".csv", std::ios::out);
        }
        for(int j = 0; j <= totalLines / (runLength * k); ++j){
            for(int i = 0; i < k; ++i){
                auto iterator = new RunIterator(input_file, startPos, bufSize, runLength, s);
                startPos += runLength * recordLength;
                iterators.push_back(iterator);
            }
            merge_runs(iterators, out_fp, output_buffer);
            for(int i = 0; i < k; ++i){
                delete iterators[i];
            }
            iterators.clear();
        }
        out_fp.close();
        if(q > 0){
            std::string unused = "output_file" + std::to_string(q - 1) + ".csv";
            remove(unused.c_str());
        }
        input_file = "output_file" + std::to_string(q) + ".csv";
        runLength *= k;
        startPos = 0;
    }
    auto endTime = system_clock::now();
    auto duration = duration_cast<microseconds>(endTime - startTime);
    std::cout << double(duration.count()) * microseconds::period::num / microseconds::period::den << std::endl;
    return 0;
}
