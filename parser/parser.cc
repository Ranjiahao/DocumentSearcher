// 预处理
// 读取并分析文档的html内容，解析出每个文档标题，url，正文（去除html标签）
// 最终把结果输出成一个行文本文件
#include <iostream>
#include <string>
#include <vector>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include "../common/util.hpp"

// 从g_input_path目录读取，将结果写入到g_output_path
std::string g_input_path = "../data/input/";
std::string g_output_path = "../data/tmp/raw_input";

// 创建文档信息结构体，表示一个html
struct DocInfo {
    std::string title;
    std::string url;
    std::string content;
};

// 枚举input_path路径下的html文件
bool EnumFile(const std::string& input_path, std::vector<std::string>* file_list) {
    namespace fs = boost::filesystem;
    fs::path root_path(input_path);
    if (!fs::exists(root_path)) {
        std::cerr << "当前目录不存在！" << std::endl;
        return false;
    }
    // 迭代器的默认构造为end迭代器
    fs::recursive_directory_iterator end_iter; 
    for (fs::recursive_directory_iterator iter(root_path); iter != end_iter; ++iter) {
        // 如果是目录文件或者不是.html文件则直接跳过
        if (!fs::is_regular(*iter) || iter->path().extension() != ".html") {
            continue;
        }
        // 将路径加入vector中
        file_list->push_back(iter->path().string());
    }
    return true;
}

// 获得html标题
bool ParseTitle(const std::string& html, std::string* title) {
    size_t beg = html.find("<title>");
    if (beg == std::string::npos) {
        std::cerr << "标题未找到！" << std::endl;
        return false;
    }
    size_t end = html.find("</title>");
    if (end == std::string::npos) {
        std::cerr << "标题未找到！" << std::endl;
        return false;
    }
    beg += std::string("<title>").size();
    *title = std::string(html, beg, end - beg);
    return true;
}

// 根据本地路径获得在线文档路径
// 本地路径: ../data/input/html/xxx.html
// 在线路径: https://www.boost.org/doc/libs/1_53_0/doc/html/xxx.html
bool ParseUrl(const std::string& file_path, std::string* url) {
    std::string url_head = "https://www.boost.org/doc/libs/1_53_0/doc/";
    std::string url_tail = std::string(file_path, g_input_path.size());
    *url = url_head + url_tail;
    return true;
}

// 去除html的标签
bool ParseContent(const std::string& html, std::string* content) {
    bool is_content = true;
    for (auto c : html) {
        if (is_content) {
            // 正文
            if (c == '<') {
                // 遇到标签
                is_content = false;
            } else {
                // 普通字符
                if (c == '\n') {
                    c = ' ';
                }
                content->push_back(c);
            }
        } else {
            // 标签
            if (c == '>') {
                // 标签结束
                is_content = true;
            }
        }
    }
    return true;
}

bool ParseFile(const std::string& file_path, DocInfo* doc_info) {
    std::string html;
    bool ret = common::Util::Read(file_path, &html);
    if (!ret) {
        std::cerr << "读取文件失败: " << file_path << std::endl;
        return false;
    }
    ret = ParseTitle(html, &doc_info->title);
    if (!ret) {
        std::cerr << "解析标题失败: " << file_path << std::endl;
        return false;
    }
    ret = ParseUrl(file_path, &doc_info->url);
    if (!ret) {
        std::cerr << "解析url失败: " << file_path << std::endl;
        return false;
    }
    ret = ParseContent(html, &doc_info->content);
    if (!ret) {
        std::cerr << "解析正文失败: " << file_path << std::endl;
        return false;
    }
    return true;
}

void WriteOutput(const DocInfo& doc_info, std::ofstream& ofstream) {
    // ofstream只能传引用或者指针
    // 使用不可用字符作为分隔符，解决粘包问题
    ofstream << doc_info.title << "\3" << doc_info.url << "\3" << doc_info.content << std::endl;
}

int main() {
    // 1. 把input目录中的所有html路径都枚举出来
    std::vector<std::string> file_list;
    bool ret = EnumFile(g_input_path, &file_list);
    if (!ret) {
        std::cerr << "枚举路径失败！" << std::endl;
        return 1;
    }
    // 打开output文件
    std::ofstream output_file(&g_output_path[0]);
    if (!output_file.is_open()) {
        std::cerr << "打开output文件失败！" << std::endl;
    }
    for (const auto& file_path : file_list) {
        // 2. 针对每个html文件单独处理
        DocInfo doc_info;
        ret = ParseFile(file_path, &doc_info);
        if (!ret) {
            std::cerr << "解析文件失败: " << file_path << std::endl;
            continue;
        }
        // 3. 把解析出来的结果写入到最终的输出文件中
        WriteOutput(doc_info, output_file);
    }
    return 0;
}
