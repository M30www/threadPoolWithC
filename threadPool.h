#ifndef _PTHREADPOOL_H_
#define _PTHREADPOOL_H_

typedef struct Threadpool Threadpool;

Threadpool* createThreadpool(int min, int max, int queueCapacity);
/**
 * @brief a func to create a threadpool
 * @param min the smallest amounts of live threads
 * @param max the largest amounts of live threass,more than min ,less than max
 * @param queueCapacity the larger amounts of task queue
 * @return an Threadpool
 * 
 */


void manager(void* arg);
/**
 * @brief it decides to kill live thread or create new thread
 * @param arg a threadpool
 * 
 * 
 */
void worker(void* arg);
/**
 * @brief it take an task from the taskQ, and then run its func
 *
 * @param arg a pool
 * 
 */
void exitThread(Threadpool* pool);

/**
 * @brief exit threads and tuen their ids to zero
 * 
 */
void threadAdd(Threadpool* pool, void (*func)(void*arg), void* arg);
/**
 * @brief insert some task into taskQ
 * 
 */
int threadPoolDestroyed(Threadpool* pool);
/**
 * @brief destroy the pool
 * 
 */

#endif