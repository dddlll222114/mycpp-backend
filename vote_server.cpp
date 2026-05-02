#pragma warning(disable:4996)
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 8888
#define BUF_SIZE 1024
#define MAX_VOTES 100
#define MAX_OPTIONS 10

typedef struct {
    char vote_id[32];
    char topic[128];
    char options[MAX_OPTIONS][64];
    int counts[MAX_OPTIONS];
    int option_count;
    char deadline[32];
    int is_closed;
} Vote;

Vote votes[MAX_VOTES];
int vote_count = 0;

// 处理客户端请求
void handle_client(SOCKET client_sock) {
    char buf[BUF_SIZE] = { 0 };
    while (1) {
        // 接收客户端指令
        int len = recv(client_sock, buf, BUF_SIZE - 1, 0);
        if (len <= 0) {
            printf("[INFO] 客户端断开连接\n");
            break;
        }
        buf[len] = '\0';
        printf("[INFO] 收到指令：%s\n", buf);

        // 1. 处理LIST指令：返回所有投票
        if (strncmp(buf, "LIST", 4) == 0) {
            char resp[BUF_SIZE] = "LIST ";
            for (int i = 0; i < vote_count; i++) {
                strcat(resp, votes[i].vote_id);
                strcat(resp, "|");
                strcat(resp, votes[i].topic);
                if (i != vote_count - 1) {
                    strcat(resp, ",");
                }
            }
            send(client_sock, resp, (int)strlen(resp), 0);
        }
        // 2. 处理CREATE指令：创建投票
        else if (strncmp(buf, "CREATE", 6) == 0) {
            char vote_id[32], topic[128], opts_str[256], deadline[32];
            sscanf(buf, "CREATE %s %s %s %s", vote_id, topic, opts_str, deadline);

            Vote* new_vote = &votes[vote_count++];
            strcpy(new_vote->vote_id, vote_id);
            strcpy(new_vote->topic, topic);
            strcpy(new_vote->deadline, deadline);
            new_vote->is_closed = 0;
            new_vote->option_count = 0;

            // 解析选项
            char* opt = strtok(opts_str, ",");
            while (opt != NULL && new_vote->option_count < MAX_OPTIONS) {
                strcpy(new_vote->options[new_vote->option_count], opt);
                new_vote->counts[new_vote->option_count] = 0;
                new_vote->option_count++;
                opt = strtok(NULL, ",");
            }
            send(client_sock, "CREATE_OK", 9, 0);
        }
        // 3. 处理VOTE指令：提交投票
        else if (strncmp(buf, "VOTE", 4) == 0) {
            char vote_id[32], opt_name[64], nick[64];
            sscanf(buf, "VOTE %s %s %s", vote_id, opt_name, nick);

            Vote* vote = NULL;
            for (int i = 0; i < vote_count; i++) {
                if (strcmp(votes[i].vote_id, vote_id) == 0) {
                    vote = &votes[i];
                    break;
                }
            }
            if (!vote || vote->is_closed) {
                send(client_sock, "VOTE_FAILED", 11, 0);
                continue;
            }

            // 找到选项并计数+1
            for (int i = 0; i < vote->option_count; i++) {
                if (strcmp(vote->options[i], opt_name) == 0) {
                    vote->counts[i]++;
                    send(client_sock, "VOTE_OK", 7, 0);
                    break;
                }
            }
        }
        // 4. 处理QUERY指令：查询投票结果
        else if (strncmp(buf, "QUERY", 5) == 0) {
            char vote_id[32];
            sscanf(buf, "QUERY %s", vote_id);

            Vote* vote = NULL;
            for (int i = 0; i < vote_count; i++) {
                if (strcmp(votes[i].vote_id, vote_id) == 0) {
                    vote = &votes[i];
                    break;
                }
            }
            if (!vote) {
                send(client_sock, "QUERY_FAILED", 12, 0);
                continue;
            }

            // 拼接结果字符串
            char resp[BUF_SIZE] = "RESULT ";
            strcat(resp, vote->vote_id);
            strcat(resp, " ");
            strcat(resp, vote->topic);
            strcat(resp, " ");
            for (int i = 0; i < vote->option_count; i++) {
                strcat(resp, vote->options[i]);
                char cnt_str[16];
                sprintf(cnt_str, ":%d", vote->counts[i]);
                strcat(resp, cnt_str);
                if (i != vote->option_count - 1) {
                    strcat(resp, ",");
                }
            }
            strcat(resp, vote->is_closed ? " CLOSED" : " OPEN");
            send(client_sock, resp, (int)strlen(resp), 0);
        }
    }
    closesocket(client_sock);
}

int main() {
    printf("=== 投票服务端启动 ===\n");

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("[ERROR] WSAStartup失败！错误码：%d\n", WSAGetLastError());
        system("pause");
        return 1;
    }
    printf("[OK] Winsock初始化成功\n");

    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock == INVALID_SOCKET) {
        printf("[ERROR] 创建Socket失败！错误码：%d\n", WSAGetLastError());
        WSACleanup();
        system("pause");
        return 1;
    }
    printf("[OK] Socket创建成功\n");

    struct sockaddr_in server_addr = { 0 };
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_sock, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("[ERROR] Bind失败！错误码：%d\n", WSAGetLastError());
        closesocket(server_sock);
        WSACleanup();
        system("pause");
        return 1;
    }
    printf("[OK] Bind成功，端口：%d\n", SERVER_PORT);

    if (listen(server_sock, 5) == SOCKET_ERROR) {
        printf("[ERROR] Listen失败！错误码：%d\n", WSAGetLastError());
        closesocket(server_sock);
        WSACleanup();
        system("pause");
        return 1;
    }
    printf("[OK] 服务端已启动，等待客户端连接...\n");

    while (1) {
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        SOCKET client_sock = accept(server_sock, (SOCKADDR*)&client_addr, &addr_len);
        if (client_sock == INVALID_SOCKET) {
            printf("[ERROR] Accept失败！错误码：%d\n", WSAGetLastError());
            continue;
        }
        printf("[INFO] 收到客户端连接！IP：%s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // 开启线程处理客户端（简单处理，也可以直接调用）
        handle_client(client_sock);
    }

    closesocket(server_sock);
    WSACleanup();
    system("pause");
    return 0;
}