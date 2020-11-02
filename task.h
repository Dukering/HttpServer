#include<pthread.h>
#ifndef __TASKQUEUE_
#define __TASKQUEUE_
struct Task{
    void (*function)(void*);
    void* arg;
};

class Task_queue{
   public: 
    /*构造函数*/
    Task_queue(int task_size);
    /*添加任务*/
    void add_task(Task* task);
    ~Task_queue();
    
    /*任务队列的首结点指针*/
    Task* Task_q; 
    /*任务队列大小*/
    int task_size;
    /*现有任务数量*/
    int task_count;
    /*任务队列的头索引*/
    int head;
    /*任务队列的尾索引*/
    int tail;
    /*任务队列的锁*/
    pthread_mutex_t key; 
    pthread_cond_t key_empty_con;
    pthread_cond_t key_full_con;
};

#endif 