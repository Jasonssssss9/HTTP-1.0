#pragma once
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>


class Sock
{
private:
    static const int backlog = 5;

public:
    // 创建监听套接字
    // 参数默认为0，代表会进入TIME_WAIT状态；不为0，代表不进入TIME_WAIT状态
    // 成功返回0，失败返回-1
    static int Socket(int opt = 0)
    {
        int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sock < 0) {
            return -1; // 出错返回-1
        }
        LOG(INFO, "listen_sock is created");

        // opt不为0，则不进入TIME_WAIT状态
        if (opt != 0){
            opt = 1;
            setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        }

        return listen_sock;
    }

    // 绑定监听套接字
    // 成功返回0，失败返回-1
    static int Bind(const int &listen_sock, const uint16_t port)
    {
        sockaddr_in local;
        bzero(&local, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_port = htons(port);
        local.sin_addr.s_addr = INADDR_ANY;
        if (bind(listen_sock, (sockaddr *)&local, sizeof(local)) < 0) {
            return -1;
        }
        return 0;
    }

    // 开始监听
    //成功返回0，失败返回-1
    static int Listen(const int &listen_sock)
    {
        if (listen(listen_sock, backlog) < 0) {
            return -1;
        }
        return 0;
    }
};
