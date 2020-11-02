#include"task.h"
#include<stdlib.h>
#include<pthread.h>
Task_queue::Task_queue(int _task_size){
    pthread_mutex_init(&key,NULL);
    pthread_cond_init(&key_empty_con,NULL);
    pthread_cond_init(&key_full_con,NULL);

    task_size = _task_size;
    task_count = 0;
    head = 0 ;
    tail = 0;
    Task_q = (Task*)malloc(sizeof(Task)*task_size);
}

void Task_queue::add_task(Task* task){
    /*先加锁，在将任务放入队列中*/
    pthread_mutex_lock(&key);

    while(task_count == task_size){
        
        pthread_cond_wait(&key_full_con,&key);
         
    }
    /*将任务放入队列中*/
    *(Task_q+tail) = *task;
    free(task);
    tail = (tail+1)%task_size;
    task_count++;

    pthread_mutex_unlock(&key);
    pthread_cond_signal(&key_empty_con);
}

Task_queue::~Task_queue(){
    /*释放任务队列内存资源和锁资源*/
    free(Task_q);
    pthread_mutex_destroy(&key);
    pthread_cond_destroy(&key_empty_con);
    pthread_cond_destroy(&key_full_con);
}