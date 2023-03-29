#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

class Util
{
public:
    //从sock中一次读一行，将内容写到输出型参数out里
    //返回out.size()，对端关闭返回0，出错返回-1
    static int Readline(int sock, std::string& out)
    {
        //一个循环，一次读一个字节，直到将本行读完
        char ch = 0;
        while(ch != '\n'){
            //这里采取阻塞读，并且必须读到\n保证把本行读完，不然会出现粘包
            ssize_t s = recv(sock, &ch, 1, 0);
            if(s > 0){
                //(1)必须把换行符\r或\r\n进行统一处理，转换为\n
                //即如果先读到一个\r，要判断到底只有一个\r还是\r\n
                if(ch == '\r'){
                    //直接读下一个内容，但是不取，看一下是不是\n，用到recv的MSG_PEEK窥探选项
                    recv(sock, &ch, 1, MSG_PEEK);
                    //如果就是\n，说明是\r\n形式，直接再把最后的\n读出来
                    if(ch == '\n'){
                        recv(sock, &ch, 1, 0);
                    }
                    //如果不是，说明只有一个\r，把这个\r变成\n
                    else{
                        ch = '\n';
                    }
                }
                //走到这ch只可能是(2)普通字符(3)\n
                out += ch;
            }
            else if(s == 0){
                //对端关闭
                return 0;
            }
            else{
                //出错
                return -1;
            }
        }
        return out.size();
    }

    //切分字符串，target为目标字符串，sep为分隔符，result_v用来保存切分结果
    static bool CutString(const std::string& target, std::vector<std::string>& result_v, std::string sep)
    {
        std::string temp(target);
        int begin = 0;
        while(true){
            int it = temp.find(sep);
            if(it == std::string::npos){
                break;
            }
            std::string t = temp.substr(begin, it-begin);
            if(t.size() > 0){
                result_v.push_back(t);
            }
            temp = temp.substr(it+sep.size());
        }
        if(temp.size() != 0){
            result_v.push_back(temp);
        }
        if(result_v.size() < 2){
            return false;
        }
        return true;
    }

    //根据一个状态码返回码描述
    static std::string Code2Desc(int code)
    {
        std::string desc;
        //为了简单只设置200和404，可以扩展
        switch(code){
            case 200:
                desc = "OK";
                break;
            case 404:
                desc = "Not Found";
                break;
            default:
                break;
        }
        return desc;
    }

    //根据一个后缀返回其对应的类型描述
    static std::string Suffix2Desc(const std::string &suffix)
    {
        static std::unordered_map<std::string, std::string> suffix2desc = {
            {".html", "text/html"},
            {".css", "text/css"},
            {".js", "application/javascript"},
            {".jpg", "application/x-jpg"},
            {".xml", "application/xml"},    
        };

        auto iter = suffix2desc.find(suffix);
        if(iter != suffix2desc.end()){
            return iter->second;
        }
        return "text/html";
    }
};