#include"Thread.h"
#include"CurrentThread.h"

#include<semaphore.h>


std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func,const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , name_(name)
    , func_(std::move(func))
{
    setDefaultName();
}
    
Thread::~Thread()
{
    if(started_ && !joined_)
    {
        thread_->detach();
    }
}

void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem,false,0);
    //开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid();//获取线程的tid值
        sem_post(&sem);//通知主线程，线程已经创建完毕
        func_();//开启一个新线程，专门执行该线程函数
    }));

    //这里必须等待获取上面获取上面新创建的线程的tid值
    sem_wait(&sem);

}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf,sizeof buf,"Thread%d",num);
        name_ = buf;
    }
}