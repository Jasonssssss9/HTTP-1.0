#pragma once 

#include <iostream>
#include "Protocol.hpp"

class Task{
    private:
        int sock;
        Callback handler; //设置回调
    public:
        Task(int _sock = -1):sock(_sock)
        {}
        //处理任务
        void Process()
        {
            //handler()实际上是一个仿函数，Task直接通过此方式调用回调机制处理请求
            handler(sock);
        }
        ~Task()
        {}
};