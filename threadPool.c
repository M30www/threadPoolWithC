#include "threadPool.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define NUMBER  2
typedef struct Task {
    void (* func)(void* arg);
    void * arg;
}Task;
/**
 * @brief one task's struct in task queue
 * 
 */

typedef struct Threadpool {
    Task* taskQ;// task queue
    pthread_t managerID;
    pthread_t* threadIDs;// an arry of threads, we put a task in a thread to let a thread work

    int queueSize;// how many thread
    int queueCapacity;
    int Front;
    int Rear;
    int maxNum;
    int minNum;
    int liveNum;
    int busyNum;
    int exitNum;

    pthread_mutex_t mutexPool;
    pthread_mutex_t mutexBusy;

    pthread_cond_t notFull;
    pthread_cond_t notEmpty;

    int shutDown;//its time to close
    

};//define a struct of Threadpool, with all sources

//create an func to init the threadpool

Threadpool* createThreadpool(int min, int max, int queueCapacity) {
    printf("try to create a pool...\n");
    Threadpool* pool = (Threadpool*)malloc(sizeof(Threadpool));//init threadpool
    printf("threadpool create successful!\n");
do{
    if (pool == NULL) {
        printf("init pool fail...\n");
        break;
    }
    //init taskQ
    pool->taskQ = (Task*)malloc(sizeof(Task) * queueCapacity);
    if (pool->taskQ == NULL) {
        printf("init taskQ fail...\n");
        break;
    }
    //init data
    printf("init pool's data...\n");
    pool->minNum = min;
    pool->maxNum = max;
    pool->busyNum = 0;
    pool->liveNum = min;
    pool->queueCapacity = queueCapacity;
    pool->shutDown = 0;
    pool->exitNum = 0;
    pool->Rear = 0;
    pool->Front = 0;
    printf("data init successful!\n");

       //init threadIDs
    pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max);
    if (pool->threadIDs == NULL) {
        printf("init threadIDs fail....\n");
        break;
    }

    memset(pool->threadIDs, 0, sizeof(pthread_t) * max);
    pthread_create(&pool->managerID, NULL, manager, pool); //manager othres
    printf("create some work threads...\n");
    for (int i = 0; i < min; ++i) {
        pthread_create(&pool->threadIDs[i], NULL, worker, pool);
    }
    //init mutex

    if (pthread_mutex_init(&pool->mutexPool, NULL) != NULL ||\
        pthread_mutex_init(&pool->mutexBusy, NULL) != NULL ||\
        pthread_cond_init(&pool->notEmpty, NULL) != NULL ||\
        pthread_cond_init(&pool->notFull, NULL) != NULL) {
            printf("init mutex fail...\n");
            break;
        }
    return pool;
} while(0);
    if (pool->threadIDs) free(pool->threadIDs);
    if (pool->taskQ) free(pool->taskQ);
    if (pool) free(pool);
    return NULL;
    
}

void manager(void* arg) {//its manager func with a void* param
    Threadpool* pool = (Threadpool*)arg;
    printf("manager thread start working...\n");
    while (!pool->shutDown) {
        sleep(3);//every 3 second, it will check the pool,and this thread run all the
        //time,until the pool changes the shutdown flag

        //take data from threadpool
        //before, we must lock the pool
        pthread_mutex_lock(&pool->mutexPool);
        int liveNum = pool->liveNum;
        int queueSize = pool->queueSize;
        pthread_mutex_unlock(&pool->mutexPool);

        pthread_mutex_lock(&pool->mutexBusy);
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexBusy);


        //to create some threads?
        if (queueSize >  liveNum && liveNum < pool->maxNum) {
            int counter= 0;
            pthread_mutex_lock(&pool->mutexPool);
            for (int i = 0; i < pool->maxNum && counter < NUMBER && pool->liveNum < pool->maxNum; ++i) {
                if (pool->threadIDs[i] == 0) {
                    counter++;
                    pthread_create(&pool->threadIDs[i], NULL, worker, pool);//there is a worker func
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }//ends when to create
        //to kill some threads?
        if (liveNum > busyNum * 2 && liveNum > pool->minNum) {
            //set number or flag
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = NUMBER;
            pthread_mutex_unlock(&pool->mutexPool);
            //sig the threads to kill themself
            for (int i = 0; i < NUMBER; ++i) {
                pthread_cond_signal(&pool->notEmpty);
            }
            
        }
    }//ends whilt
    return;
}


void worker(void* arg) {
    Threadpool* pool = (Threadpool*)arg;
    printf("an busy work thread add...\n");
    while (1) {
        pthread_mutex_lock(&pool->mutexPool);
        while (pool->queueSize == 0 && !pool->shutDown) {
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
            if (pool->exitNum > 0) {
                pool->exitNum--;
                if (pool->liveNum > pool->minNum) {
                    pool->liveNum--;
                    printf("exit extra thread...\n");
                    pthread_mutex_unlock(&pool->mutexPool);
                    exitThread(pool);
                }
            }
        }//cond while
        if (pool->shutDown) {
            pthread_mutex_unlock(&pool->mutexPool);
            exitThread(pool);
        }
        Task task;
        task.func = pool->taskQ[pool->Front].func;
        task.arg = pool->taskQ[pool->Front].arg;
        pool->Front = (pool->Front + 1) % pool->queueCapacity;
        pool->queueSize--;
        pthread_cond_signal(&pool->notFull);
        pthread_mutex_unlock(&pool->mutexPool);
        printf("thread %ld start working...\n", pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);
        task.func(task.arg);
        free(task.arg);
        task.arg = NULL;
        printf("thread %ld end working....\n", pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }
    return;
}

//add some task into taskQ
void threadAdd(Threadpool* pool, void (*func)(void*), void* arg) {
    pthread_mutex_lock(&pool->mutexPool);
    while (pool->queueSize == pool->queueCapacity && !pool->shutDown) {
        pthread_cond_wait(&pool->notFull, &pool->mutexPool);
    }
    if (pool->shutDown) {
        pthread_mutex_unlock(&pool->mutexPool);
        return;
    }
    pool->taskQ[pool->Rear].func = func;
    pool->taskQ[pool->Rear].arg = arg;
    pool->Rear = (pool->Rear + 1) % pool->queueCapacity;
    pool->queueSize++;
    pthread_cond_signal(&pool->notEmpty);//notify
    pthread_mutex_unlock(&pool->mutexPool);
}

//its time to exit thread
void exitThread(Threadpool* pool) {
    pthread_t tid = pthread_self();
    for (int i = 0; i < pool->maxNum; ++i) {
        if (pool->threadIDs[i] == tid) {
            pool->threadIDs[i] = 0;
            printf("exitThread() called, thread %ld end...\n", tid);
            pthread_exit(NULL);
        }
    }
}

int threadPoolDestroyed(Threadpool* pool) {
    //switch off the pool
    if (pool == NULL) return -1;
    pool->shutDown = 1;
    //collect manager thread
    printf("wait 4 the managerID shut down...\n");
    pthread_join(pool->managerID, NULL);
    printf("the manager thread shutdow successfully!\n");
    //destroy live threads
    printf("destory all the live thread...\n");
    for (int i = 0; i < pool->liveNum; ++i) {
        pthread_cond_signal(&pool->notEmpty);
    }
    //destroy the taskQ
    if (pool->taskQ) {
        printf("release taskQ...\n");
        free(pool->taskQ);
        pool->taskQ = NULL;
    }
    if (pool->threadIDs) {
        printf("release threadIDs...\n");
        free(pool->threadIDs);
        pool->threadIDs = NULL;
    }
    //destroy the mutex
    printf("release mutex....\n");
    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    printf("relese cond...\n");
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);
    printf("desroty the pool...\n");
    free(pool);
    pool = NULL;
    return 1;
}

