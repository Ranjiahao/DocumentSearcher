#include <algorithm>
#include <boost/algorithm/string/case_conv.hpp>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include "searcher.h"
#include "../common/util.hpp"

namespace searcher {
/*
 * Index模块
 */

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



/*
 * Searcher模块
 */

bool Searcher::Init(const std::string& input_path) {
    return index->Build(input_path);
}

bool Searcher::Search(const std::string& query, std::string* output) {
    // 对查询词进行分词
    std::vector<std::string> tokens;
    index->CutWord(query, &tokens);
    // 根据分词结果查倒排，并将倒排拉链合并在all_token_result中
    InvertedList all_token_result;
    for (auto word : tokens) {
        boost::to_lower(word);
        const InvertedList* inverted_list = index->GetInvertedList(word);
        if (inverted_list == nullptr) {
            // 该词在倒排索引中不存在
            continue;
        }
        all_token_result.insert(all_token_result.end(), inverted_list->begin(), inverted_list->end());
    }
    // 根据权重进行降序排序
    std::sort(all_token_result.begin(), all_token_result.end(), [](const Weight& w1, const Weight& w2) {
        return w1.weight > w2.weight;
    });
    // 根据倒排拉链中的文档id，查正排
    // 将doc_info中的内容构造成json格式
    Json::Value results;
    for (const auto& weight : all_token_result) {
        const DocInfo* doc_info = index->GetDocInfo(weight.doc_id);
        Json::Value result;
        result["title"] = doc_info->title;
        result["url"] = doc_info->url;
        result["desc"] = GenerateDesc(doc_info->content, weight.word);
        results.append(result);
    }
    // 将json对象results序列化成字符串，写入output中
    Json::FastWriter writer;
    *output = writer.write(results);
    return true;
}

std::string Searcher::GenerateDesc(const std::string& content, const std::string& word) {
    // 找到word在正文中出现的位置
    size_t first_pos = content.find(word);
    size_t desc_beg = 0;
    if (first_pos == std::string::npos) {
        // word只出现再标题中
        if (content.size() < 160) {
            return content;
        }
        std::string desc(content, 0, 160);
        desc[desc.size() - 1] = '.';
        desc[desc.size() - 2] = '.';
        desc[desc.size() - 3] = '.';
        return desc;
    }
    // 找到了first_pos位置
    desc_beg = first_pos < 60 ? 0 : first_pos - 60;
    if (desc_beg + 160 >= content.size()) {
        // desc_beg后面的内容不够160直接移动到末尾即可
        return std::string(content, desc_beg);
    } else {
        std::string desc(content, desc_beg, 160);
        desc[desc.size() - 1] = '.';
        desc[desc.size() - 2] = '.';
        desc[desc.size() - 3] = '.';
        return desc;
    }
}
} // namespace searcher
