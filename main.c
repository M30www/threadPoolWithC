#include <stdio.h>
#include "threadPool.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>


void taskFunc(void* arg) {
    int num = *(int*)arg;
    printf("thread %ld is workding, number = %d\n", pthread_self(), num);
    sleep(1);
}

int main() {
    Threadpool* pool = createThreadpool(3, 10, 100);

    for (int i = 0; i < 100; ++i) {
        int* num = (int*)malloc(sizeof(int));
        *num = i + 100;
        threadAdd(pool, taskFunc, num);
    }
    printf("the tasks int queue is %d");
    sleep(40);
    threadPoolDestroyed(pool);
    
    return;
}

