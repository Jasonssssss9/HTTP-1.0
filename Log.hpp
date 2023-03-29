#pragma once
#include <iostream>
#include <string>
#include <ctime>

#define LOG(level, message) Log(#level, message, __FILE__, __LINE__)

static void Log(std::string level, std::string message, std::string file_name, int line)
{
    std::cout << "[" << level << "]" << "[" << time(nullptr) << "]" << "[" << message << "]" << "[" << file_name << "]" << "[" << line << "]" << std::endl;
}
