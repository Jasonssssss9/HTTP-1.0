#include "TCPServer.hpp"
#include "ThreadPool.hpp"

TcpServer* TcpServer::pt_ = nullptr;
ThreadPool* ThreadPool::single_instance = nullptr;
