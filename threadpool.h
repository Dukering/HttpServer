#ifndef __PTHREADPOOL_
#define __PTHREADPOOL_
#include<pthread.h>
#include<stdlib.h>
#include"task.h"

class pthread_pool{
 public:  
   pthread_pool(int _pthread_count,Task_queue* _task_queue);
   static void* pthread_task(void*);
   void* pthread_destroy();
   ~pthread_pool();
   /*任务队列*/
   Task_queue* task_queue;
   /*线程队列*/
   pthread_t* pthread_queue;
   int pthread_size;
   int pthread_count;

};

#endif