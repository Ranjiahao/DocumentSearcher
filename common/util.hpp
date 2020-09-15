#pragma once

#include <fstream>
#include <boost/algorithm/string.hpp>

namespace common {
class Util {
public:
    // 从指定的路径中读取出文件内容到output中
    static bool Read(const std::string& input_path, std::string* output) {
        std::ifstream file(&input_path[0]);
        if (!file.is_open()) {
            return false;
        }
        std::string line;
        while (std::getline(file, line)) {
            *output += (line + "\n");
        }
        file.close();
        return true;
    }

    // 封住boost中的字符串切分
    static void Split(const std::string& input, const std::string& delimiter, std::vector<std::string>* output) {
        // aaa\3bbb\3\3ccc  按照\3切分
        // token_compress_off: aaa bbb ccc
        // token_compress_on: aaa bbb "" ccc
        boost::split(*output, input, boost::is_any_of(delimiter), boost::token_compress_off);
    }
};
} // namespace common
