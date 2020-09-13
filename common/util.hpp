#pragma once

#include <fstream>

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
};
} // namespace common
