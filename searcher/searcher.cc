#include "searcher.h"
#include "../common/util.hpp"

namespace searcher {
const char* const DICT_PATH = "../jieba_dict/jieba.dict.utf8";    
const char* const HMM_PATH = "../jieba_dict/hmm_model.utf8";    
const char* const USER_DICT_PATH = "../jieba_dict/user.dict.utf8";    
const char* const IDF_PATH = "../jieba_dict/idf.utf8";
const char* const STOP_WORD_PATH = "../jieba_dict/stop_words.utf8";

Index::Index()
    : jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH) {}
const DocInfo* Index::GetDocInfo(int64_t doc_id) {
    if (doc_id < 0 || (size_t)doc_id >= forward_index.size()) {
        return nullptr;
    }
    return &forward_index[doc_id];
}

const InvertedList* Index::GetInvertedList(const std::string& key) {
    std::unordered_map<std::string, InvertedList>::iterator it = inverted_index.find(key);
    if (it == inverted_index.end()) {
        return nullptr;
    }
    return &it->second;
}

bool Index::Build(const std::string& input_path) {
    std::fstream file(&input_path[0]);
    if (!file.is_open()) {
        std::cerr << "raw_input文件打开失败！" << std::endl;
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        DocInfo* doc_info = BuildForward(line);
        if (doc_info == nullptr) {
            std::cerr << "构建正排失败！" << std::endl;
            continue;
        }
        BuildInverted(*doc_info);
        // 每构建100个索引打印一次，减少与磁盘的交互频率
        if (doc_info->doc_id % 100 == 0) {
            std::cerr << doc_info->doc_id << std::endl;
        }
    }
    file.close();
    return true;
}

DocInfo* Index::BuildForward(const std::string& line) {
    std::vector<std::string> tokens;
    common::Util::Split(line, "\3", &tokens);
    if (tokens.size() != 3) {
        std::cerr << "切分字符串失败！" << std::endl;
        return nullptr;
    }
    DocInfo doc_info;
    doc_info.doc_id = forward_index.size();
    doc_info.title  = tokens[0];
    doc_info.url = tokens[1];
    doc_info.content = tokens[2];
    forward_index.push_back(std::move(doc_info));
    return &forward_index[forward_index.size() - 1];
}

void Index::BuildInverted(const DocInfo& doc_info) {
    // 统计词频结构体
    struct WordCnt {
        int title_cnt = 0;
        int content_cnt = 0;
    };
    std::unordered_map<std::string, WordCnt> word_cnt_map;
    // 针对标题进行分词，统计标题词频
    std::vector<std::string> title_token;
    CutWord(doc_info.title, &title_token);
    for (auto word : title_token) {
        // 统一转成小写
        boost::to_lower(word);
        ++word_cnt_map[word].title_cnt;
    }
    // 针对正文进行分词，统计正文词频
    std::vector<std::string> content_token;
    CutWord(doc_info.content, &content_token);
    for (auto word : content_token) {
        boost::to_lower(word);
        ++word_cnt_map[word].content_cnt;
    }
    // 根据统计结果，整合出Weight对象，并把结果更新到倒排索引中
    for (const auto& word_pair : word_cnt_map) {
        Weight weight;
        weight.doc_id = doc_info.doc_id;
        weight.weight = 10 * word_pair.second.title_cnt + word_pair.second.content_cnt;
        weight.word = word_pair.first;
        // 找到倒排拉链并追加到拉链末尾
        InvertedList& inverted_list = inverted_index[word_pair.first];
        inverted_list.push_back(std::move(weight));
    }
}

void Index::CutWord(const std::string& input, std::vector<std::string>* output) {
    jieba.CutForSearch(input, *output);
}
} // namespace searcher
