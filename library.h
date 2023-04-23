#ifndef LIBRARY_H
#define LIBRARY_H

#include <cstdio>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <unordered_map>


struct Attribute {
    std::string name;
    int length;
    std::string type;

    Attribute(std::string name, int length, std::string type) : name(name), length(length), type(type) {};
};

struct Schema {
    std::vector<std::unique_ptr<Attribute>> attrs;
    std::vector<int> sort_attrs;  // 根据哪几个attrs进行排序
};


struct Record {
    Schema *schema;
    std::vector<std::string> data;

    Record(const std::string &data, Schema *schema) {
        this->schema = schema;
        std::stringstream ss(data);
        std::string temp;
        while (std::getline(ss, temp, ',')) {
            this->data.emplace_back(temp);
        }
    };
};

void printSchema(const std::unique_ptr<Schema> &s) {
    for (auto &i: s->attrs) {
        std::cout << "{name : " << i->name << ", length : " << i->length << ", type : " << i->type << "}" << std::endl;
    }
    std::cout << "Sort attributes: ";
    for (auto i: s->sort_attrs) {
        std::cout << s->attrs[i]->name << " ";
    }
    std::cout << std::endl;
}

static bool compare(const std::unique_ptr<Record> &s1, const std::unique_ptr<Record> &s2) {
    for (auto i: s1->schema->sort_attrs) {
        std::string type = s1->schema->attrs[i]->type;
        if (type == "integer") {
            if (std::stoi(s1->data[i]) == std::stoi(s2->data[i]))
                continue;
            return std::stoi(s1->data[i]) > std::stoi(s2->data[i]);
        } else if (type == "float") {
            if (std::stof(s1->data[i]) == std::stof(s2->data[i]))
                continue;
            return std::stof(s1->data[i]) > std::stof(s2->data[i]);
        } else {
            if (s1->data[i] == s2->data[i])
                continue;
            return s1->data[i] > s2->data[i];
        }
    }
    // 如果全部相同，则随便排序
    return false;
}

static bool compareS(const std::unique_ptr<Record> &s1, const std::unique_ptr<Record> &s2) {
    for (auto i: s1->schema->sort_attrs) {
        std::string type = s1->schema->attrs[i]->type;
        if (type == "integer") {
            if (std::stoi(s1->data[i]) == std::stoi(s2->data[i]))
                continue;
            return std::stoi(s1->data[i]) < std::stoi(s2->data[i]);
        } else if (type == "float") {
            if (std::stof(s1->data[i]) == std::stof(s2->data[i]))
                continue;
            return std::stof(s1->data[i]) < std::stof(s2->data[i]);
        } else {
            if (s1->data[i] == s2->data[i])
                continue;
            return s1->data[i] < s2->data[i];
        }
    }
    // 如果全部相同，则随便排序
    return false;
}


void mk_runs(std::vector<std::unique_ptr<Record>> &_records) {
    std::sort(_records.begin(), _records.end(), compare);
}


class RunIterator {

public:

    // buffer
    std::vector<std::unique_ptr<Record>> _records;
    long pos;       // 文件读取位置
    long haveRunLength; //  当前读过的record个数
    long bufSize;   // 能放入的record个数
    long runLength; // 一共要读取的record个数
    std::string f_name;
    Schema *s;

    RunIterator(const std::string &f_name, long startPos, long bufSize, long runLength, std::unique_ptr<Schema>& s) {
        // init buffer, get numOfRecords records
        this->s = s.get();
        this->f_name = f_name;
        this->pos = startPos;
        this->bufSize = bufSize;
        this->runLength = runLength;
        this->haveRunLength = 0;
        this->_records.reserve(bufSize);
    }

    void fillBuffer() {
        // fill with new data
        std::string temp;
        std::ifstream fp(f_name, std::ios::in);
        fp.seekg(this->pos, fp.beg);
        long needRead = std::min(this->bufSize, this->runLength - this->haveRunLength);
        if (needRead > 0) {
            for (long i = 0; i < needRead && std::getline(fp, temp); ++i) {
                this->_records.push_back(std::make_unique<Record>(temp, this->s));
                this->haveRunLength += 1;
            }
            pos = fp.tellg();
            mk_runs(this->_records);
        }
        fp.close();
    }

};


std::string vecToString(const std::unique_ptr<Record> &record) {
    std::string ans;
    int len = record->data.size();
    for(int i = 0; i < len; ++i){
        if(i == len - 1){
            ans += record->data[i];
        }
        else{
            ans += record->data[i];
            ans += ',';
        }
    }
    return ans;
}

void merge_runs(std::vector<RunIterator *> &iterators, std::ofstream &out_fp, long buf_size) {
    std::vector<std::string> buf;
    int numOfEmpty = 0;
    for (auto i: iterators) {
        if (i->_records.empty()){
            i->fillBuffer();
            if(i->_records.empty()) numOfEmpty++;
        }
    }
    // 当iterator中的所有_record都为空时退出函数
    while (numOfEmpty < iterators.size()) {
        // find minimum
        int index = -1;
        bool first = true;
        for (int i = 0; i < iterators.size(); ++i) {
            if (first && !iterators[i]->_records.empty()) {
                index = i;
                first = false;
            } else if (!iterators[i]->_records.empty() &&
                       compareS(iterators[i]->_records.back(), iterators[index]->_records.back())) {
                index = i;
            }
        }

        // convert to string and add to buffer
        buf.push_back(vecToString(iterators[index]->_records.back()));

        // erase the record, if iterator is empty stop using it
        iterators[index]->_records.pop_back();
        if (iterators[index]->_records.empty()) {
            iterators[index]->fillBuffer();
            if(iterators[index]->_records.empty()) numOfEmpty++;
        }

        // when buffer is full, write to disk
        if (buf.size() >= buf_size) {
            for (const auto &i: buf) { out_fp << i << std::endl; }
            buf.clear();
        }
    }
    // 添加剩余内容到disk中
    if (!buf.empty()) {
        for (const auto &i: buf) { out_fp << i << std::endl; }
        buf.clear();
    }
}

#endif