#pragma once
#include "Log.hpp"
#include "Socket.hpp"
#include <unistd.h>
#include <pthread.h>

class TcpServer
{
private:
    uint16_t port_;
    int listen_sock_;
    static TcpServer* pt_;

    TcpServer(uint16_t port):port_(port), listen_sock_(-1){}
public:
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    static TcpServer* GetTCPInstance(uint16_t port)
    {
        static pthread_mutex_t pt_mutex = PTHREAD_MUTEX_INITIALIZER;
        if(pt_ == nullptr){
            pthread_mutex_lock(&pt_mutex);
            if(pt_ == nullptr){
                pt_ = new TcpServer(port);
                //创建监听套接字
                pt_->listen_sock_ = Sock::Socket(1);
                if(pt_->listen_sock_ == -1){
                    LOG(FATAL, "Socket error");
                    exit(2);
                }
                LOG(INFO, "Socket is successful");
                //绑定
                if(Sock::Bind(pt_->listen_sock_, port) == -1){
                    LOG(FATAL, "Bind error");
                    exit(3);
                }
                LOG(INFO, "Bind is successful");
                //监听
                if(Sock::Listen(pt_->listen_sock_) < 0){
                    LOG(FATAL, "Listen error");
                    exit(4);
                }
                LOG(INFO, "Listen is successful");
            }
            pthread_mutex_unlock(&pt_mutex);
        }
        return pt_;
    }

    static void DelTCPInstance()
    {
        static pthread_mutex_t pt_mutex = PTHREAD_MUTEX_INITIALIZER;    
        if(pt_ != nullptr){
            pthread_mutex_lock(&pt_mutex);
            if(pt_ != nullptr){
                if(pt_->listen_sock_ >= 0){
                    close(pt_->listen_sock_);
                }
                delete pt_;
            }
            pthread_mutex_unlock(&pt_mutex);
        }
    }

    //返回listen_sock_值
    int GetSock()
    {
        //外部accept获取连接时需要监听套接字
        int ret = listen_sock_;
        return ret;
    }

};