#include"threadpool.h"
#include"task.h"
#include"util.h"
#include<stdio.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include"request.h"
extern pthread_mutex_t qlock;
extern std::priority_queue<timer*,std::deque<timer*>,compare> timer_queue; 
/*socket请求处理函数*/
void handlevent(void* arg){
     request* req_ = (request*)arg;
     req_->handleResponse();
}
/*每一秒钟，来释放无效或者超时的长连接*/                       /*这里有个问题每秒钟都会加锁，进行判断，是否可用双检锁？*/
void* timer_listen(void* fun){
     while(true){
          pthread_mutex_lock(&qlock);
          while(!timer_queue.empty()){
          timer* top_timer = timer_queue.top();
          if(top_timer->IsDelete()){
               timer_queue.pop();
               delete top_timer;
          } 
          else if(!top_timer->isValid()){
               timer_queue.pop();
               delete top_timer;
          } 
          else
          {
               break;
          }
          
          }
          pthread_mutex_unlock(&qlock);
          sleep(1);
     }
} 
/*fork_1*/
int main(){
   
    Task_queue* task_q = new Task_queue(1024);
    pthread_pool* pthread_p =  new pthread_pool(100,task_q);
    pthread_t alarm_time;
    pthread_create(&alarm_time,NULL,timer_listen,NULL);
    /*socket注册*/
    int sfd = socket(AF_INET,SOCK_STREAM,0);
    /*消除bind时"Address already in use"错误*/
    int optval = 1;
    if(setsockopt(sfd, SOL_SOCKET,  SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        return -1;
   /*绑定端口和ip*/
   struct sockaddr_in server_addr;
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   server_addr.sin_port = htons((unsigned short)8000);
   if(bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        return -1;
   /*启动监听*/

   if(listen(sfd, 1024) == -1)
        return -1;
   /*将sfd设为非阻塞，在accept时能够不阻塞*/     
   setSocketNonBlocking(sfd); 
   /*创建epoll*/
   int efd = epoll_create(10);
   
   /*将listen描述符加入epoll*/
   epoll_event event;
   
   event.events = EPOLLIN | EPOLLET;
   request* req = new request(sfd);
   event.data.ptr = (void*) req;
   if((epoll_ctl(efd,EPOLL_CTL_ADD,sfd,&event))<0)
       printf("error epoll_add!");
   
   epoll_event* get_events = (epoll_event*)malloc(sizeof(epoll_event)*10);
   while(true){ 
      /*等待符合条件的描述符*/
       int events_num = epoll_wait(efd,get_events,10,-1);
        
      /*将唤醒的描述符分配给多线程执行*/
       for(int i=0;i<events_num;i++){
            request* cur_req = (request*)((get_events+i)->data.ptr);
            /*如果是监听描述符，则将其放入epoll中*/
            if( cur_req->getFd() == sfd){
               struct sockaddr_in client_addr;
               //memset(&client_addr, 0, sizeof(struct sockaddr_in));
               socklen_t client_addr_len = 0;
               int accept_fd = 0; 
               
          
               int IPSTRSIZE = 40;
               char ipstr[IPSTRSIZE];
               // 设为非阻塞模式
               
               //int ret = setSocketNonBlocking(accept_fd);
               while((accept_fd = accept(sfd,(struct sockaddr*)&client_addr, &client_addr_len)) > 0 ){   
                   //inet_ntop(AF_INET,&client_addr.sin_addr.s_addr,ipstr,IPSTRSIZE);
                   printf("new_acc_fd:%d\n",accept_fd);
                   epoll_event _epo_event;
                   _epo_event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                   request* ac_req = new request(accept_fd,efd,"/home/dl/test/");

                   timer* cur_timer = new timer(ac_req,KEEP_ALIVE_TIME);
                   ac_req->set_timer(cur_timer);
                   pthread_mutex_lock(&qlock);
                   timer_queue.push(cur_timer);
                   pthread_mutex_unlock(&qlock);
                   _epo_event.data.ptr = (void*) ac_req;
                   epoll_ctl(efd,EPOLL_CTL_ADD, accept_fd,&_epo_event);    
                   
               }  
               
            }
            else{
                /*如果是接受描述符，则将任务放入队列*/
                printf("%d\n",cur_req->getFd());
                /*由于有请求，所以将req从timer_queue去除*/
                cur_req->seperateTimmer();

                Task* cur_task = (Task*)malloc(sizeof(Task));
                cur_task->function = handlevent;
                cur_task->arg = (void*)cur_req;
                task_q->add_task(cur_task);
            }
            

       } 

   }
   
   close(sfd);
   pthread_p->pthread_destroy();
   return 0;
}