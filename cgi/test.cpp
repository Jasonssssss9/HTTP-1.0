#include <iostream>
#include "cgi.hpp"
#include <string>

//CGI实现业务逻辑，可能进行某种计算(计算，搜索，登陆等)，进行某种存储(注册，上传)等
//对方的数据一定是a=x1&b=x2，x1和x2为两个数字，只需要将这两个数字进行四则运算返回给对方即可
int main()
{
    //获取数据
    std::string query_string;
    GetQueryString(query_string);
    //a=100&b=200
    //std::cerr << "############query_string: " << query_string << std::endl;
    
    //数据处理
    std::string str1;
    std::string str2;
    CutString(query_string, "&", str1, str2);

    std::string name1;
    std::string value1;
    CutString(str1, "=", name1, value1);

    std::string name2;
    std::string value2;
    CutString(str2, "=", name2, value2);

    //将以下内容输出给浏览器 -> 标准输出
    // std::cout << name1 << " : " << value1 << std::endl;
    // std::cout << name2 << " : " << value2 << std::endl;

    //为了调试而打印
    std::cerr << name1 << " : " << value1 << std::endl;
    std::cerr << name2 << " : " << value2 << std::endl;
    int x = atoi(value1.c_str());
    int y = atoi(value2.c_str());
    
    if(y == 0){
        return 1;
    }

    //这里就直接将两个数进行四则运算，给对方返回一个html文件
    //html文件需要在CGI程序中就写好
    std::cout << "<html>";
    std::cout << "<head><meta charset=\"utf-8\"></head>";
    std::cout << "<body>";
    std::cout << "<h3> " << value1 << " + " << value2 << " = "<< x+y << "</h3>";
    std::cout << "<h3> " << value1 << " - " << value2 << " = "<< x-y << "</h3>";
    std::cout << "<h3> " << value1 << " * " << value2 << " = "<< x*y << "</h3>";
    std::cout << "<h3> " << value1 << " / " << value2 << " = "<< x/y << "</h3>";
    std::cout << "</body>";
    std::cout << "</html>";

    return 0;
}
