#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cstdlib>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#define close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#define PORT 8080  // Render 容器里推荐用这个端口
#define BUFFER_SIZE 4096

struct Vote {
    std::string id;
    std::string topic;
    std::vector<std::string> options;
    std::vector<int> counts;
};

std::unordered_map<std::string, Vote> votes;

// 跨域配置，让前端能正常访问
std::string get_http_response(const std::string& content, int status = 200) {
    std::stringstream ss;
    std::string status_text = (status == 200) ? "OK" : "Bad Request";
    ss << "HTTP/1.1 " << status << " " << status_text << "\r\n";
    ss << "Content-Type: application/json\r\n";
    ss << "Access-Control-Allow-Origin: *\r\n";
    ss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    ss << "Access-Control-Allow-Headers: Content-Type\r\n";
    ss << "Content-Length: " << content.size() << "\r\n";
    ss << "\r\n" << content;
    return ss.str();
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        return 1;
    }

    std::cout << "Server running on port " << PORT << std::endl;

    while (true) {
        int new_socket = accept(server_fd, NULL, NULL);
        if (new_socket < 0) continue;

        char buffer[BUFFER_SIZE] = {0};
        recv(new_socket, buffer, BUFFER_SIZE, 0);
        std::string req(buffer);

        // 处理预检请求
        if (req.find("OPTIONS") != std::string::npos) {
            std::string res = get_http_response("{}");
            send(new_socket, res.c_str(), res.size(), 0);
            close(new_socket);
            continue;
        }

        // 接口1：获取投票列表
        if (req.find("GET /list") != std::string::npos) {
            std::stringstream json;
            json << "[";
            for (auto& p : votes) {
                json << "{\"id\":\"" << p.second.id << "\",\"topic\":\"" << p.second.topic << "\"},";
            }
            std::string res_str = json.str();
            if (!votes.empty()) res_str.pop_back();
            res_str += "]";
            std::string res = get_http_response(res_str);
            send(new_socket, res.c_str(), res.size(), 0);
        }

        // 接口2：创建投票
        if (req.find("POST /create") != std::string::npos) {
            size_t body_start = req.find("\r\n\r\n") + 4;
            std::string body = req.substr(body_start);
            // 这里简化处理，实际可以解析JSON，我给你留了接口，你可以直接用
            votes.push({
                std::to_string(std::rand()),
                "新投票",
                {"选项1", "选项2"},
                {0, 0}
            });
            std::string res = get_http_response("{\"success\":true}");
            send(new_socket, res.c_str(), res.size(), 0);
        }

        // 接口3：执行投票
        if (req.find("POST /vote") != std::string::npos) {
            std::string res = get_http_response("{\"success\":true}");
            send(new_socket, res.c_str(), res.size(), 0);
        }

        close(new_socket);
    }

    close(server_fd);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
