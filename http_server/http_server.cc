#include "cpp-httplib/httplib.h"
#include "../searcher/searcher.h"

int main() {
    // 创建Searcher对象
    searcher::Searcher searcher;
    bool ret = searcher.Init("../data/tmp/raw_input");
    if (!ret) {
        std::cout << "searcher初始化失败！" << std::endl;
        return 1;
    }
    // 创建server对象
    httplib::Server server;
    server.Get("/searcher", [&searcher](const httplib::Request& req, httplib::Response& resp) {
        if (!req.has_param("query")) {
            resp.set_content("请求参数错误", "text/plain; charset=utf-8");
            return;
        }
        std::string query = req.get_param_value("query");
        std::string results;
        searcher.Search(query, &results);
        resp.set_content(results, "application/json; charset=utf-8");
    });
    server.set_mount_point("/", "./wwwroot");
    // 启动服务器
    server.listen("0.0.0.0", 10001);
    return 0;
}
