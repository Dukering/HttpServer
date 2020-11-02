#include"threadpool.h"
#include<stdio.h>
/*线程池构造函数初始化*/
pthread_pool::pthread_pool(int _pthread_size,Task_queue* _task_queue){
     /*任务队列首地址*/
     task_queue = _task_queue;
     /*设置线程池的线程容量和现有量*/
     pthread_size = _pthread_size;
     pthread_count = 0;
     pthread_queue = (pthread_t*)malloc(sizeof(pthread_t)*pthread_size);
     
    
     /*为线程分配任务*/
     for(int i=0;i<pthread_size;i++){
       
        if(pthread_create(pthread_queue+i,NULL,pthread_task,this)!=0){           //pthread_task必须为静态方法
            printf("pthread_create error!");
            pthread_destroy();

        }   
        pthread_count++;     
        
     }
} 

void* pthread_pool::pthread_task(void* _phread_pool){

     pthread_pool* cur_pthread_pool = (pthread_pool*)_phread_pool;
     Task cur_task;
     while(true){
         /*先对任务队列加锁，来取任务*/
         pthread_mutex_lock(&cur_pthread_pool->task_queue->key);
         while(cur_pthread_pool->task_queue->task_count == 0 ){     //why while 因为在pthread_cond_singal()和pthread_cond_wait()返回之间有时间差
             
             pthread_cond_wait(&(cur_pthread_pool->task_queue->key_empty_con),&(cur_pthread_pool->task_queue->key));
             
         }
         
         cur_pthread_pool->task_queue->task_count--;
         /*从任务队列头部取出任务函数和任务参数*/
         cur_task.function = cur_pthread_pool->task_queue->Task_q[cur_pthread_pool->task_queue->head].function;
         cur_task.arg = cur_pthread_pool->task_queue->Task_q[cur_pthread_pool->task_queue->head].arg;
          
         cur_pthread_pool->task_queue->head = (cur_pthread_pool->task_queue->head+1)
                                                  %cur_pthread_pool->task_queue->task_size;
         pthread_mutex_unlock(&cur_pthread_pool->task_queue->key);   
         pthread_cond_signal(&cur_pthread_pool->task_queue->key_full_con);
         /*执行任务*/
         cur_task.function(cur_task.arg);  
     }   

     return NULL;

}
void* pthread_pool::pthread_destroy(){
    /*线程的回收*/
    for(int i=0;i<pthread_count;i++){

        pthread_join(*(pthread_queue+i),NULL);

    }
    
}
pthread_pool::~pthread_pool(){
    /*线程的回收*/
    for(int i=0;i<pthread_size;i++){

        pthread_join(*(pthread_queue+i),NULL);
    
    }
    free(pthread_queue);   

}