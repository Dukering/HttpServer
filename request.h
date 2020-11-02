#ifndef __REQUEST_
#define __REQUEST_
#include<string>
#include<unordered_map>
#include<sys/epoll.h>
#include<queue>
const int BUFMAX = 1024*1024;
const int METHOD_POST = 2;
const int METHOD_GET = 1;
const int ANALYZE_URI_ERROR = -1;
const int ANALYZE_URI_SUCCESS = 1;
const int ANALYZE_HEADERS_ERROR = -1;
const int ANALYZE_HEADERS_SUCCESS = 1;
const int CREATE_RESPONSE_ERROR = -1;
const int CREATE_PESPONSE_SUCCESS = 1;
const int HTTP10 = 1.0;
const int HTTP11 = 1.1; 
const int KEEP_ALIVE_TIME = 5000;
  
class timer;
class request{
        private:
            /*文件描述符*/     
            int fd;
            /*该文件描述符注册在某个epoll上的epoll文件描述符*/
            int epoll_fd;
            /*http请求内容*/
            std::string content;
            /*申请资源的当前文件路径*/
            std::string cur_path;
            /*该文件描述符的连接时间信息*/
            timer* _timer;       
            /*
            请求行
            */
            /*请求方法*/
            int method;
            /*请求资源的相对路径*/
            std::string path;
            /*http版本号*/
            int HTTPversion;
            /*是否持久连接*/
            bool keep_alive;
            /*请求头*/
            std::unordered_map<std::string,std::string> headers;
        private:
            /*请求行获取*/
            int analyz_URI();
            /*请求体解析*/
            int analyz_Headers();
            /*构造响应内容*/
            int create_Response();

            void reset();
        public:
            request(int _fd);
            request(int _fd,int _epoll_fd,std::string _cur_path);
            ~request();
            int getFd();
            void set_timer(timer* t);
            void handleResponse(); 
            void seperateTimmer();
};
/*资源文件名的映射*/
class MimeType{
  public:  
   static std::unordered_map<std::string,std::string> Mime; 
   static std::unordered_map<std::string,std::string> initial_mime(){
       std::unordered_map<std::string,std::string> mime;
       mime[".html"] = "text/html";
       mime[".avi"] = "video/x-msvideo";
       mime[".bmp"] = "image/bmp";
       mime[".c"] = "text/plain";
       mime[".doc"] = "application/msword";
       mime[".gif"] = "image/gif";
       mime[".gz"] = "application/x-gzip";
       mime[".htm"] = "text/html";
       mime[".ico"] = "application/x-ico";
       mime[".jpg"] = "image/jpeg";
       mime[".png"] = "image/png";
       mime[".txt"] = "text/plain";
       mime[".mp3"] = "audio/mp3";
       mime["default"] = "text/html";
       return mime;
   }
};

/*每个socket的连接时间信息类*/
class timer{
    private:
      /*绑定的socket请求*/
      request* req;
      /*长连接断开时的时间戳*/
      size_t timeout_tamp;
      /*该socket请求是否已经断开*/
      bool isDelete; 
    public:
      timer(request* _req,int timeout);
      ~timer();
      /*获取长连接断开时的时间戳*/
      size_t get_timeout()const;
      /*处理socket触发事件时，将绑定的socket与timer分离*/
      void clearReq(); 
      /*判断timer中是否挂着socket*/
      bool IsDelete(); 
      /*判断是否超过长连接的时间*/  
      bool isValid();
};
class compare{ 
    public:
      bool operator()(const timer*a,const timer*b)const;
};
#endif
 