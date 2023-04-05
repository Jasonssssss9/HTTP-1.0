#pragma once
#include "Log.hpp"
#include "Util.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>

#define HTTP_VERSION "HTTP/1.0"

#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define SERVER_ERROR 500

#define WEB_ROOT "wwwroot"      //当前路径下的wwwroot目录作为虚拟根目录
#define HOME_PAGE "index.html"  //首页文件
#define HTTP_VERSION "HTTP/1.0" //默认HTTP版本
#define LINE_END "\r\n"         //默认换行符
#define PAGE_404 "404.html"     //默认404页面
#define PAGE_400 "400.html"     //默认400页面
#define PAGE_500 "500.html"     //默认500页面


//Http请求结构体
struct HttpRequest
{
public:
    //读取请求，包括一个HTTP报文四个部分
    std::string reqLine_; //请求行
    std::vector<std::string> reqHead_; //请求报头，多行
    std::string blank_; //空行
    std::string reqBody_; //数据

    //请求解析内容
    //由请求行解析得到
    std::string method_; //请求类型，只支持GET和POST
    std::string version_; //HTTP版本
    std::string uri_; //path?arg

    std::string path_; //请求文件路径
    std::string argu_; //GET方法uri中的arg请求部分

    
    //由请求报头解析得到
    std::unordered_map<std::string, std::string> headerMap_; //建立报头的键值关系
    int contentLen_; //数据部分长度，目的是确定正文部分数据大小，实际上是reqBody_.size()
    std::string suffix_; //请求资源的后缀

    bool cgi_; //判断是否设置CGI机制
    int size_; //被请求资源文件的大小

    //path_ argu_ suffix_ cgi_ size_这5个参数都要在构建响应时才能最终确定，并且用于构建响应

    HttpRequest():contentLen_(0), cgi_(false), size_(0){}
};

//Http响应结构体
struct HttpResponse
{
public:
    //需要写入响应报文的内容
    std::string statusLine_; //状态行
    std::vector<std::string> resHeader_; //响应报头
    std::string blank_; //空行
    std::string resBody_; //

    //其他内容
    int statusCode_; //状态码
    int fd_;         //打开资源文件的文件描述符

    HttpResponse():blank_(LINE_END), statusCode_(0), fd_(-1){}
};

//EndPoint类为HTTP服务器的核心，用来读取请求，分析请求，并构建响应
class EndPoint
{
private:
    int sock_; //每个连接的线程都会对应一个EndPoint，socket即其标识
    HttpRequest httpReq_;  //用来保存读取与分析响应的结果
    HttpResponse httpRes_; //用来保存构建的响应
    bool stop_;  //作为终止符，可能出错或对端关闭

private:
    //读取请求行，要求前面的内容都已经读完
    bool RecvHttpReqLine()
    {
        auto& line = httpReq_.reqLine_; //为了简单
        int ret = Util::Readline(sock_, line);
        if(ret > 0){
            line.resize(line.size()-1); //借助resize去掉结尾的'\n'
            LOG(INFO, line);
        }
        else{
            stop_ = true;
        }
        return stop_;
    }

    //读取请求报头，要求前面内容已经读完
    bool RecvHttpReqHeader()
    {
        auto& header = httpReq_.reqHead_;
        std::string line;
        while(true){
            line.clear();
            int ret = Util::Readline(sock_, line);
            if(ret < 0){
                stop_ = true;
                break;
            }
            if(line == "\n"){
                //读取完毕
                httpReq_.blank_ = "\n";
                break;
            }
            line.resize(line.size()-1);
            header.push_back(line);
            LOG(INFO, line);
        }
        return stop_;
    }

    //解析请求行
    void ParseHttpReqLine()
    {
        //请求行是一个以空格为分隔的字符串，可以借助stringstream分隔
        std::stringstream ss(httpReq_.reqLine_);
        ss >> httpReq_.method_ >> httpReq_.uri_ >> httpReq_.version_;
        //之后把method全部转换为大写
        auto& method = httpReq_.method_;
        std::transform(method.begin(), method.end(), method.begin(), ::toupper);
        // for test
        // LOG(INFO, httpReq_.method_);
        // LOG(INFO, httpReq_.uri_);
        // LOG(INFO, httpReq_.version_);
    }

    //解析请求报头
    void ParseHttpReqHeader()
    {
        //请求报头每一个条目都为key-value结构，中间用": "作为分隔符
        std::string key;
        std::string value;
        for(auto& header : httpReq_.reqHead_){
            std::stringstream ss(header);
            ss >> key >> value;
            key.resize(key.size()-1);
            httpReq_.headerMap_.insert(make_pair(key, value));
            // for test
            // LOG(INFO, key);
            // LOG(INFO, value);
        }
    }

    //判断是否需要读取正文数据，如果需要还要修改contentlen_成员
    bool IfNeedRecvBody()
    {
        auto& method = httpReq_.method_;
        //只有POST才用读取body
        if(method == "POST"){
            //并且需要查看Content-Length报头内容，大于0才需要读取
            auto& map = httpReq_.headerMap_;
            auto it = map.find(std::string("Content-Length"));
            if(it != map.end()){
                httpReq_.contentLen_ = atoi(it->second.c_str());
                LOG(INFO, "Post Method, Content-Length: "+it->second);
                return true;
            }
        }
        return false;
    }

    //接收正文数据，默认前面的数据都已经取完
    bool RecvHttpReqBody()
    {
        if(IfNeedRecvBody()){
            int len = httpReq_.contentLen_;
            auto& body = httpReq_.reqBody_;
            char ch = 0;
            //这里采取阻塞读，必须保证把len读完，因为可能存在粘包问题
            while(len){
                ssize_t ret = recv(sock_, &ch, 1, 0);
                if(ret > 0){
                    body += ch;
                    len--;
                }
                else{
                    stop_ = true;
                    break;
                }
            }
            LOG(INFO, "body");
        }
        return stop_;
    }

    //CGI
    int ProcessCgi()
    {
        LOG(INFO, "CGI is beginning");

        int code = OK;
        auto& method = httpReq_.method_;
        auto& argu = httpReq_.argu_;         //GET数据
        auto& body_data = httpReq_.reqBody_; //POST数据
        auto& content_len = httpReq_.contentLen_;
        auto& bin = httpReq_.path_; //bin指需要运行的CGI程序，就是path_中存储的路径对应文件
        auto& res_body = httpRes_.resBody_;

        std::string method_env; //请求方法
        std::string query_env; //表示GET方法请求数据
        std::string content_len_env; //表示POST方法正文长度

        //创建两个匿名管道，站在父进程角度
        int input[2];
        int output[2];

        if(pipe(input) < 0){
            LOG(ERROR, "input pipe error");
            code = SERVER_ERROR;
            return code;
        }
        if(pipe(output) < 0){
            LOG(ERROR, "output pipe error");
            code = SERVER_ERROR;
            return code;
        }

        //当前新线程创建子进程
        pid_t pid = fork();
        if(pid == 0){
            //child:
            //关闭管道对应文件描述符
            close(input[0]);
            close(output[1]);

            //设置mothod作为环境变量
            method_env += "METHOD=";
            method_env += method;
            putenv((char*)method_env.c_str());

            //判断是GET方法还是POST方法，分别进行环境变量设置
            if(method == "GET"){
                query_env += "QUERY_STRING=";
                query_env += argu;
                putenv((char*)query_env.c_str());
                LOG(INFO, "GET method, add QUERY_STRING env");
            }
            else if(method == "POST"){
                content_len_env += "CONTENT_LENGTH=";
                content_len_env += std::to_string(content_len);
                putenv((char*)content_len_env.c_str());
                LOG(INFO, "POST method, add CONTENT_LENGTH env");
            }
            else{}  

            LOG(INFO, std::string("bin: ")+bin);

            //子进程进行重定向
            //注意重定向一定要放到整个代码后面，不然前面的日志信息都没法输出了
            dup2(output[0], 0);
            dup2(input[1], 1);

            //继续程序替换，运行的程序就是bin
            execl(bin.c_str(), bin.c_str(), nullptr);
        }
        else if(pid > 0){
            //father:
            //关闭对应文件描述符
            close(input[1]);
            close(output[0]);

            //如果是POST方法，父进程还需要将正文数据通过管道传过去
            //如果POST正文数据太大，一次write不一定能发送完，因此需要考虑多次发送
            if(method == "POST"){
                const char* start = body_data.c_str(); //需要发送数据的开始
                size_t total = 0; //已经发送的数据总数，需要和content_len比较
                size_t size = 0; //send函数返回值
                //while循环条件：total<content_len为还需要继续发送，size>0为发送成功
                while(total < content_len && (size = write(output[1], start+total, body_data.size()-total)) > 0){
                    total += size;
                }
            }

            //从子进程读取内容，一个字节为单位读取
            char ch = 0;
            while(read(input[0], &ch, 1) > 0){
                res_body += ch;
            }

            //等待子进程
            int status = 0;
            pid_t ret = waitpid(pid, &status, 0);
            if(ret == pid){
                //WIFEXITED(status)非0，表示子进程正常退出
                if(WIFEXITED(status)){
                    //如果子进程正常退出，就可以使用WEXITSTATUS获取子进程的退出码
                    //例如子进程exit(0);退出，则WEXITSTATUS(status)就是0
                    if(WEXITSTATUS(status) == 0){
                        code = OK;
                    }
                    else{
                        code = BAD_REQUEST;
                    }
                }
                else{
                    code = SERVER_ERROR;
                }
            }
            LOG(INFO, "CIG process quit");

            //关闭文件描述符
            close(input[0]);
            close(output[1]);
            //子进程不用关闭，因为子进程退出资源自然销毁
            //并且子进程推出后read也会自动停止，保证上面的read不会阻塞
        }
        else{
            LOG(ERROR, "fork error");
            code = SERVER_ERROR;
            return code;
        }
        return code;
    }

    //非CGI
    int ProcessNonCgi()
    {
        auto& fd = httpRes_.fd_;
        //直接打开文件即可
        fd = open(httpReq_.path_.c_str(), O_RDONLY);
        if(fd >= 0){
            LOG(INFO, httpReq_.path_ + " open success!");
            return OK;
        }
        return NOT_FOUND;
    }

    void BuildOkResponse()
    {
        //OK响应中，需要加入Content-Type和Content-Length两个条目
        auto& header = httpRes_.resHeader_;
        auto& body = httpRes_.resBody_;

        std::string line("Content-Type: ");
        line += Util::Suffix2Desc(httpReq_.suffix_);
        line += LINE_END;
        header.push_back(line);

        line = "Content-Length: ";
        //注意，设置CGI机制时才会用到body，这时才有body的大小；不用时没有body，就是资源文件大小
        if(httpReq_.cgi_){
            line += std::to_string(body.size());
            LOG(INFO, std::string("Content length: ")+std::to_string(body.size()));
        }
        else{
            line += std::to_string(httpReq_.size_);
            LOG(INFO, std::string("Content length: ")+std::to_string(httpReq_.size_));
        }
        line += LINE_END;
        header.push_back(line);
        LOG(INFO, "BuildOkResponse is successful");
    }

    void HandlerError(std::string page)
    {
        // std::cout << "debug: " << page << std::endl;
        LOG(INFO, std::string("handler error, path: ")+page);
        httpReq_.cgi_ = false;
        //要给用户返回对应的404、400、500页面
        httpRes_.fd_ = open(page.c_str(), O_RDONLY);
        if(httpRes_.fd_ > 0){
            struct stat st;
            stat(page.c_str(), &st);
            httpReq_.size_ = st.st_size;

            std::string line = "Content-Type: text/html";
            line += LINE_END;
            httpRes_.resHeader_.push_back(line);

            line = "Content-Length: ";
            line += std::to_string(st.st_size);
            line += LINE_END;
            httpRes_.resHeader_.push_back(line);
        }
    }

    //封装为一个报文
    void BuildHttpResHelper()
    {
        auto& sline = httpRes_.statusLine_;
        auto& code = httpRes_.statusCode_;


        //构建状态行
        sline += HTTP_VERSION;
        sline += " ";
        sline += std::to_string(code);
        sline += " ";
        sline += Util::Code2Desc(code);
        sline += LINE_END;

        LOG(INFO, std::string("code: ")+std::to_string(code));

        //构建响应报头
        std::string path = WEB_ROOT;
        path += "/";
        switch(code){
        //为了简单起见，将NOT_FOUND BAD_REQUEST SERVER_ERROR都作为404处理
            case OK:
                BuildOkResponse();
                break;
            case NOT_FOUND:
                path += PAGE_404;
                HandlerError(path);
                break;
            case BAD_REQUEST:
                path += PAGE_400;
                HandlerError(path);
                break;
            case SERVER_ERROR:
                path += PAGE_500;
                HandlerError(path);
                break;
            default:
                break;
        }
    }

public:
    EndPoint(int sock):sock_(sock), stop_(false)
    {}

    ~EndPoint()
    {
        close(sock_);
    }
    

    bool IsStop()
    {
        return stop_;
    }

    //读取并分析HTTP请求
    void RecvHttpRequest()
    {
        //分别完成读取请求行，读取请求头部，之后分析请求行与请求头部，最后读取请求数据
        if( (!RecvHttpReqLine()) && (!RecvHttpReqHeader()) ){
            ParseHttpReqLine();
            ParseHttpReqHeader();
            RecvHttpReqBody();
        }
    }

    //构建响应报文
    void BuildHttpResponse()
    {
        //根据已经读取和分析完的请求报文，构建响应报文
        
        auto& method = httpReq_.method_;
        auto& code = httpRes_.statusCode_;
        auto& uri = httpReq_.uri_;
        auto& path = httpReq_.path_;
        auto& argu = httpReq_.argu_;
        auto& suffix = httpReq_.suffix_;
        
        //根据不同的请求设置不同的相应方式
        if(method != "GET" && method != "POST"){
            //如果method等于其他方法，认为非法
            LOG(WARNING, "Wrong method");
            std::cout << "method is " << method << std::endl;
            code = BAD_REQUEST;
        }
        else{
            //如果method为正确方法，进入正确逻辑
            if(method == "GET"){
                //判断uri是否有参数，如果有设置CGI机制
                LOG(INFO, "GET method");
                auto it = uri.find("?");
                if(it != std::string::npos){
                    //说明uri有参数，设置启用CGI机制
                    httpReq_.cgi_ = true;
                    //需要将uri分为path和argu两部分
                    std::vector<std::string> v;
                    Util::CutString(uri, v, "?");
                    path = v[0];
                    argu = v[1];
                    LOG(INFO, "GET CGI enabled");
                    LOG(INFO, std::string("argu: ")+argu);
                }
                else{
                    //说明uri无参数，不启用CGI机制，并且确定path就是uri
                    path = uri;
                }
            }
            else if(method == "POST"){
                //确定使用CGI机制，并且path就是uri
                httpReq_.cgi_ = true;
                path = uri;
                LOG(INFO, "POST method");
            }

            //获取真实路径并判断处理
            std::string vPath = path;
            path = WEB_ROOT;
            path += vPath;
            LOG(INFO, std::string("path: ")+path);

            //对请求资源的判断和处理
            //如果对方发来的只是一个根目录
            //还可能是，对方给一个目录的最后还加了/，这种情况说明对方访问的一定还是目录，因此响应首页
            if(path[path.size()-1] == '/'){
                path += HOME_PAGE;
            }
            struct stat st; //stat结构体和函数同名，因此必须加struct
            //判断请求资源是否存在
            if(stat(path.c_str(), &st) != 0){
                //资源不存在
                code = NOT_FOUND;
                LOG(WARNING, path + " Not Found");
            }
            else{
                //资源存在，进入正确逻辑
                //判断资源是否是目录
                if(S_ISDIR(st.st_mode)){
                    //如果是目录，返回首页，但是肯定不以/结尾，因为此情况上面处理过了
                    path += "/";
                    path += HOME_PAGE;
                    //很易错，这里path改变了，必须要再stat一次，获取新的st
                    stat(path.c_str(), &st);
                    LOG(WARNING, "access directory");
                }
                else{
                    //如果不是目录，但有可执行权限
                    if( (st.st_mode&S_IXUSR) || (st.st_mode&S_IXGRP) || (st.st_mode&S_IXOTH) ){
                        //设置CGI
                        httpReq_.cgi_ = true;
                    }   
                    //其他文件，不需要在此处设置CGI
                    //最后获取该资源得大小
                }
                httpReq_.size_ = st.st_size;

                //进行资源后缀判断
                int found = path.rfind(".");
                if(found == std::string::npos){
                    //如果没找到，说明path是目录，资源为HOME_PAGE
                    suffix = ".html";
                }
                else{
                    //找到，说明给定文件后缀
                    suffix = path.substr(found);
                }

                //进行CGI或非CGI处理
                if(httpReq_.cgi_){
                    //执行CGI目标程序，获取结果resBody
                    code = ProcessCgi(); 
                }
                else{
                    code = ProcessNonCgi(); //简单的网页返回，返回静态网页,只需要打开即可
                }

            }
        }
        //封装响应报头
        BuildHttpResHelper();
    }

    //发送响应报文
    void SendHttpResponse()
    {
        //发送响应状态行
        auto& sline = httpRes_.statusLine_;
        send(sock_, sline.c_str(), sline.size(), 0);
        std::string sline_tem = sline;
        sline_tem.pop_back(); //防止打印的时候还有\n
        //LOG(INFO, std::string("Send status line: ")+sline_tem);

        //发送响应报头
        auto& header = httpRes_.resHeader_;
        for(auto e : header){
            send(sock_, e.c_str(), e.size(), 0);
        }
        //LOG(INFO, "Send header");

        //发送空行
        auto& blank = httpRes_.blank_;
        send(sock_, blank.c_str(), blank.size(), 0);

        //发送正文
        if(httpReq_.cgi_){
            //发送body
            auto& body = httpRes_.resBody_;
            const char* start = body.c_str();
            ssize_t total = 0;
            ssize_t size = 0;

            while(total < body.size() && (size = send(sock_, start+total, body.size()-total, 0)) > 0){
                total += size;
            }

            LOG(INFO, "send data successfully");

        }
        else{
            //直接通过sendfile文件，把资源文件发送到socket里，不需要上层的缓冲区
            LOG(INFO, "sendfile");
            sendfile(sock_, httpRes_.fd_, nullptr, httpReq_.size_);
            close(httpRes_.fd_);
        }
    }
};

class Callback
{
public:
    Callback(){}

    //设置一个仿函数，实现回调机制，这样一个Task就可以调用Handler处理请求
    void operator()(int sock)
    {
        Handler(sock);
    }

    void Handler(int sock)
    {
        LOG(INFO, "handler is beginning");

        EndPoint* pe  = new EndPoint(sock);
        pe->RecvHttpRequest();
        if(!pe->IsStop()){
            LOG(INFO, "No receive error, begin to build and send a response");
            pe->BuildHttpResponse();
            pe->SendHttpResponse();
        }
        else{
            LOG(WRONG, "Receive error");
        }

        delete pe;
        close(sock);
    }
};