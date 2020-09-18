#include <iostream>
#include "searcher.h"

int main() {
    searcher::Searcher searcher;
    bool ret = searcher.Init("../data/tmp/raw_input");
    if (!ret) {
        std::cout << "Searcher初始化失败！" << std::endl;
        return 1;
    }
    while (true) {
        std::cout << "searcher> " << std::flush;
        std::string query;
        std::cin >> query;
        if (!std::cin.good()) {
            // 读到 EOF
            break;
        }
        std::string results;
        searcher.Search(query, &results);
        std::cout << results << std::endl;
    }
    return 0;
}
