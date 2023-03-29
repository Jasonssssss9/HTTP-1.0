#pragma once
#include "TCPServer.hpp"
#include "Log.hpp"
#include "Protocol.hpp"
#include <pthread.h>
#include <iostream>
#include <string>

#define PORT 8081

class HttpServer
{
private:
    uint16_t port_;
    bool stop;
    TcpServer* pt_;
public:
    HttpServer(uint16_t port = PORT):port_(port), stop(false), pt_(nullptr)
    {
        pt_ = TcpServer::GetTCPInstance(port_);
    }

    ~HttpServer()
    {
        TcpServer::DelTCPInstance();
        pt_ = nullptr;
    }

    void Loop()
    {
        LOG(INFO, "Loop is beginning");
        while(!stop){
            int listen_sock = pt_->GetSock();
            sockaddr_in peer;
            socklen_t peer_size;
            int sock = accept(listen_sock, (sockaddr*)&peer, &peer_size);
            if(sock < 0){
                continue;
            }
            LOG(INFO, "get a new link");
            //创建新线程，要把sock作为参数到pthread_create里
            int *sock_new = new int(sock);
            pthread_t tid;
            pthread_create(&tid, nullptr, Entry::Handler, sock_new);
            pthread_detach(tid);

        }
    }
};