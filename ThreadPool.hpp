#pragma once
#include <iostream>
#include <queue>
#include <pthread.h>
#include "Log.hpp"
#include "Task.hpp"

#define NUM 6

class ThreadPool
{
private:
    int num_;
    std::queue<Task> task_queue_;
    pthread_mutex_t lock_;
    pthread_cond_t cond_;

    ThreadPool(int num = NUM) : num_(num)
    {
        pthread_mutex_init(&lock_, nullptr);
        pthread_cond_init(&cond_, nullptr);

        for (int i = 0; i < num_; i++)
        {
            pthread_t tid;
            if (pthread_create(&tid, nullptr, ThreadRoutine, this) != 0)
            {
                LOG(FATAL, "create thread pool error!");
            }
        }
        LOG(INFO, "create thread pool success!");
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    static ThreadPool *single_instance;

public:
    static ThreadPool *Getinstance()
    {
        static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
        if (single_instance == nullptr)
        {
            pthread_mutex_lock(&_mutex);
            if (single_instance == nullptr)
            {
                single_instance = new ThreadPool();
            }
            pthread_mutex_unlock(&_mutex);
        }
        return single_instance;
    }

    bool TaskQueueIsEmpty()
    {
        return task_queue_.size() == 0 ? true : false;
    }

    void Lock()
    {
        pthread_mutex_lock(&lock_);
    }

    void Unlock()
    {
        pthread_mutex_unlock(&lock_);
    }

    void ThreadWait()
    {
        //进入等待，并且释放锁
        pthread_cond_wait(&cond_, &lock_);
    }
    
    void ThreadWakeup()
    {
        //唤醒线程
        pthread_cond_signal(&cond_);
    }

    static void *ThreadRoutine(void *args)
    {
        ThreadPool *tp = (ThreadPool *)args;

        while (true)
        {
            Task t;
            tp->Lock();
            //在任务队列为空的条件下，线程进入休眠
            //注意使用while循环，防止伪唤醒
            while (tp->TaskQueueIsEmpty())
            {
                tp->ThreadWait();
            }
            tp->PopTask(t);
            tp->Unlock();
            t.Process();
        }
    }

    //外部给任务队列放入任务
    void PushTask(const Task &task) // in
    {
        Lock();
        task_queue_.push(task);
        Unlock();
        //当任务队列有任务，唤醒一个线程
        ThreadWakeup();
    }

    //线程池从任务队列取走任务
    void PopTask(Task &task) // out
    {
        task = task_queue_.front();
        task_queue_.pop();
    }

    ~ThreadPool()
    {
        pthread_mutex_destroy(&lock_);
        pthread_cond_destroy(&cond_);
    }
};