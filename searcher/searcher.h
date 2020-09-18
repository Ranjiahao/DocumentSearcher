#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "cppjieba/Jieba.hpp"

namespace searcher {
/*
 * 索引模块
 * */

// 用于构建正排索引
struct DocInfo {
    int64_t doc_id;
    std::string title;
    std::string url;
    std::string content;
};

// 用于构建倒排索引
struct Weight {
    int64_t doc_id;
    int weight;
    std::string word;
};

// 倒排拉链
typedef std::vector<Weight> InvertedList;

// 表示整个索引结构，并向外部提供API
class Index {
public:
    Index();
    // 查正排，返回指针，NULL表示未找到
    const DocInfo* GetDocInfo(int64_t doc_id);
    // 查倒排，返回指针，NULL表示未找到
    const InvertedList* GetInvertedList(const std::string& key);
    // 构建索引
    bool Build(const std::string& input_path);
    // 分词函数
    void CutWord(const std::string& input, std::vector<std::string>* output);
private:
    // 构造正排索引
    DocInfo* BuildForward(const std::string& line);
    // 构造倒排索引
    void BuildInverted(const DocInfo& doc_info);
    // 正排索引，数组下标就对应到doc_id
    std::vector<DocInfo> forward_index;
    // 倒排索引，使用一个hash表示映射关系
    std::unordered_map<std::string, InvertedList> inverted_index;
    cppjieba::Jieba jieba;
};



/*
 * 搜索模块
 * */

class Searcher {
public:
    Searcher()
        : index(new Index()) {}
    bool Init(const std::string& input_path);
    bool Search(const std::string& query, std::string* results);
private:
    Index* index;
    // 生成文章描述
    std::string GenerateDesc(const std::string& content, const std::string& word);
};
} // namespace searcher
