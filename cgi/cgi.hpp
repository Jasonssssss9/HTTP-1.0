#pragma once
#include <unistd.h>
#include <iostream>
#include <string>
#include <cstdlib>

//获取请求数据，参数data为输出型参数，返回正文
static bool GetQueryString(std::string& data)
{
    bool result = true;
    //先获取METHOD环境变量
    std::string method = getenv("METHOD");
    if(method == "GET"){
        data = getenv("QUERY_STRING");
    }
    else if(method == "POST"){
        int content_len = atoi(getenv("CONTENT_LENGTH"));
        char ch = 0;
        while(content_len){
            read(0, &ch, 1);
            data += ch;
            content_len--;
        }
    }
    else{
        result = false;
    }
    return result;
}

//把一个in字符串以sep为分隔符分割为out1和out2两部分
static void CutString(std::string &in, const std::string &sep, std::string &out1, std::string &out2)
{
    auto pos = in.find(sep);
    if(std::string::npos != pos){
        out1 = in.substr(0, pos);
        out2 = in.substr(pos+sep.size());
    }
}

