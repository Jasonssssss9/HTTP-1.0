#pragma once
#include "TCPServer.hpp"
#include "Log.hpp"
#include "Task.hpp"
#include "ThreadPool.hpp"
#include <pthread.h>
#include <signal.h>
#include <pthread.h>
#include <iostream>
#include <string>


#define PORT 8081

class HttpServer
{
private:
    uint16_t port_;
    bool stop;
public:
    HttpServer(uint16_t port = PORT):port_(port), stop(false)
    {
        //忽略写错误出现时系统发送的SIGPIPE信号
        signal(SIGPIPE, SIG_IGN); 
    }

    ~HttpServer()
    {
        TcpServer::DelTCPInstance();
    }

    void Loop()
    {
        TcpServer* pt = TcpServer::GetTCPInstance(port_);
        LOG(INFO, "Loop is beginning");
        while(!stop){
            int listen_sock = pt->GetSock();
            sockaddr_in peer;
            socklen_t peer_size;
            int sock = accept(listen_sock, (sockaddr*)&peer, &peer_size);
            if(sock < 0){
                continue;
            }
            LOG(INFO, "get a new link");
            Task task(sock);
            ThreadPool::Getinstance()->PushTask(task);
        }
    }
};