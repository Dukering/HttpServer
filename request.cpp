#include"request.h"
#include<unistd.h>
#include<sys/stat.h>
#include<string.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<sys/time.h>
std::unordered_map<std::string,std::string> MimeType::Mime = MimeType::initial_mime();
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
std::priority_queue<timer*,std::deque<timer*>,compare> timer_queue; 
request::request(int _fd):fd(_fd),keep_alive(false){}
request::request(int _fd,int _epoll_fd,std::string _cur_path):fd(_fd),epoll_fd(_epoll_fd),cur_path(_cur_path),keep_alive(false){}
int request::analyz_URI(){
    int pos = content.find('\r',0);
    if(pos<0)
       return ANALYZE_URI_ERROR;
    /*请求行内容*/   
    std::string request_line = content.substr(0,pos);
    
    if(content.size() > pos+1)
       content = content.substr(pos+2);
    else
       content.clear();
    
    pos = request_line.find("GET");
    /*获取请求方式*/
    if(pos<0){
        pos = request_line.find("POST");
        if(pos<0)
           return ANALYZE_URI_ERROR;
        else
           method = METHOD_POST;   
    }
    else
        method = METHOD_GET;    
    /*获取资源路径*/
    pos = request_line.find("/",pos);
    if(pos<0)
       return  ANALYZE_URI_ERROR;
    else{
        int end_pos = request_line.find(" ",pos);
        if(pos<0)
           return ANALYZE_URI_ERROR;
        else{
            path = request_line.substr(pos+1,end_pos-pos-1);
            /*若是get方式，则需要去掉参数部分*/
            int __pos = path.find("?");
            if(__pos>=0)
               path = path.substr(0,__pos);
        }
        pos = end_pos;   
    }
    /*获取http版本号*/
    pos = request_line.find("/",pos);
    if(pos<0)
      return  ANALYZE_URI_ERROR;
    else{
        if(request_line.size()-pos<=3)
           return  ANALYZE_URI_ERROR;
        else{
            std::string http = request_line.substr(pos + 1,3);
            if(http == "1.0")
               HTTPversion = HTTP10;
            else if(http == "1.1")
               HTTPversion = HTTP11;
            else
               return ANALYZE_URI_ERROR;      
        }   
    }  
    return ANALYZE_URI_SUCCESS;
}
/*
请求体信息获取
*/
int request::analyz_Headers(){
      while(true){
          int pos = content.find('\r');
          std::string cur_str = content.substr(0,pos); 
          if(cur_str.size() == 0)
            break;
          if(content.size() > pos+1)
              content = content.substr(pos+2);
          else
              content.clear();

          int key_end = cur_str.find(":",0);
          if(key_end<0)
            return ANALYZE_HEADERS_ERROR;
          std::string key = cur_str.substr(0,key_end);
          int con_start = cur_str.find(" ",key_end);
          if(con_start<0)
            return ANALYZE_HEADERS_ERROR;
          std::string con = cur_str.substr(con_start+1,cur_str.size());
          
          
          headers[key]=con;
      }
      return ANALYZE_HEADERS_SUCCESS;
    
};
/*
构造http响应的内容
*/
int request::create_Response(){
    if(method == METHOD_POST){
        char header[BUFMAX] = {0};
        /*构造响应头*/
        sprintf(header,"HTTP/1.1 %d %s\r\n",200,"OK");
        /*是否进行长链接*/
        if(headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive" )
        {
                keep_alive = true;
                sprintf(header, "%sConnection: keep-alive\r\n", header);
                sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, KEEP_ALIVE_TIME);
        } 
        char *send_content = "I have receiced this.";

        sprintf(header, "%sContent-length: %zu\r\n", header, strlen(send_content));
        sprintf(header, "%s\r\n", header);
        size_t send_len = (size_t)write(fd, header, strlen(header));
        if(send_len != strlen(header))
        {
            perror("Send header failed");
            return CREATE_RESPONSE_ERROR;
        }
        
        send_len = (size_t)write(fd, send_content, strlen(send_content));
        if(send_len != strlen(send_content))
        {
            perror("Send content failed");
            return CREATE_RESPONSE_ERROR;
        }
        return CREATE_PESPONSE_SUCCESS;
    }
    else if(method == METHOD_GET){
        char header[BUFMAX];
        /*构造响应头*/
        sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
        /*是否进行长链接*/
        if(headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive")
        {
            keep_alive = true;
            sprintf(header, "%sConnection: keep-alive\r\n", header);
            sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, KEEP_ALIVE_TIME);
        }
        
        int dot_pos = path.find('.');
        const char* filetype;
        if (dot_pos < 0) 
            filetype = MimeType::Mime["default"].c_str();
        else
            filetype = MimeType::Mime[path.substr(dot_pos)].c_str();
        /*查找资源文件*/
        struct stat sbuf;
        if (stat((cur_path+path).c_str(), &sbuf) < 0)
        {
            //handleError(fd, 404, "Not Found!");
            return CREATE_RESPONSE_ERROR;
        }
        /*构造文件类型*/
        sprintf(header, "%sContent-type: %s\r\n", header, filetype);
        // 通过Content-length返回文件大小
        sprintf(header, "%sContent-length: %ld\r\n", header, sbuf.st_size);
        sprintf(header, "%s\r\n", header);
        /*将响应内容写入文件描述符*/
        size_t send_len = (size_t)write(fd, header, strlen(header));
        if(send_len != strlen(header))
        {
            perror("Send header failed");
            return CREATE_RESPONSE_ERROR;
        }
        /*打开资源文件*/
        int src_fd = open((cur_path+path).c_str(), O_RDONLY, 0);  //相对路径相加
        /*利用共享内存复制资源文件*/
        char *src_addr = static_cast<char*>(mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0));
        close(src_fd);
        // 发送文件并校验完整性
        send_len = write(fd, src_addr, sbuf.st_size);
        if(send_len != sbuf.st_size)
        {
            perror("Send file failed");
            return CREATE_RESPONSE_ERROR;
        }
        /*关闭释放共享内存*/
        munmap(src_addr, sbuf.st_size);
        
        return CREATE_PESPONSE_SUCCESS;  

    }
    else
       return CREATE_RESPONSE_ERROR;
    

};

void request::handleResponse(){
     
     char buf[BUFMAX]={0};
     ssize_t read_num = read(fd,buf,sizeof(buf));
     std::string read_con(buf,buf+read_num);
     
     content = read_con; 
     /*请求行解析*/
     int flag = analyz_URI();
     bool isError = 0;
     while(1){
            if(flag == ANALYZE_URI_ERROR)
            {
                perror("URI_ERROR");
                isError = 1;
                break;
            }
            /*  请求头解析*/
            flag = analyz_Headers();
            if(flag == ANALYZE_HEADERS_ERROR)
            {
                perror("HEADER_ERROR");
                isError = 1;
                break;
            }
            /*构造响应内容并将内容写入描述符文件*/
            flag = create_Response(); 
            if(flag == CREATE_RESPONSE_ERROR)
            {
                perror("RESPONSE_ERROR");
                isError = 1;
                break;
            }
            break;
    }
     if(isError){
         delete this;
         return;
     }
     

    if(keep_alive){
         reset();
     }
    else{
         delete this;
         return;
     }

    timer* cur_timer = new timer(this, KEEP_ALIVE_TIME);
    _timer = cur_timer;
    /*将该连接放入长连接优先队列*/
    pthread_mutex_lock(&qlock);
    timer_queue.push(cur_timer);
    pthread_mutex_unlock(&qlock); 
    
    epoll_event _epo_event; 
    _epo_event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;   //EPOLLONESHOT使得一个socket被epoll触发后，就被epoll删除，之后需要重新注册。这样可以避免多个线程同时对一个socket的多个触发事件进行处理
    _epo_event.data.ptr = this;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &_epo_event);
    
      
}; 
void request::reset(){
    /*删除请求内容*/
    content.clear();
    /*删除资源路径*/
    path.clear();
    /*删除请求体*/
    headers.clear();
    keep_alive = false;
}
int request::getFd(){
    return fd;
}
request::~request(){
    struct epoll_event ev;
    // 超时的一定都是读请求，没有"被动"写。
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.ptr = (void*)this;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &ev);
    close(fd);
}
/*保存该socket请求的连接时间类信息指针*/
void request::set_timer(timer* t){
    _timer = t;
}
/*在线程执行socket请求时，将该时间信息类脱钩*/
void request::seperateTimmer(){
    _timer->clearReq();
    _timer = NULL;
}

timer::timer(request* _req,int _timeout):req(_req),isDelete(false){
    struct timeval now;
    gettimeofday(&now, NULL);
    // 以毫秒计
    timeout_tamp = ((now.tv_sec * 1000) + (now.tv_usec / 1000)) + _timeout;

};
/*长连接时间超时时，释放socket连接*/
timer::~timer(){
     delete req;    
}
size_t timer::get_timeout()const{
    return timeout_tamp;
}
bool timer::IsDelete(){
    return isDelete;
}
/*判断锁socket连接是否超时*/
bool timer::isValid(){
    struct timeval now;
    gettimeofday(&now, NULL);
    size_t temp = ((now.tv_sec * 1000) + (now.tv_usec / 1000));  //毫秒为单位
    if (temp < timeout_tamp)
    {
        return true;
    }
    else
    {
        return false;
    }
}
/*脱钩函数*/
void timer::clearReq(){
    req = NULL;
    isDelete = true;
}
bool compare::operator()(const timer* a,const timer* b )const{
   return a->get_timeout()>b->get_timeout();
}