#pragma once

#include"noncopeable.h"

#include<string>
#include<vector>
#include<functional>
#include<memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop *baseloop,const std::string&nameArg);
    ~EventLoopThreadPool();
    
    //设置底层线程数量
    void setThreadNum(int num){numThreads_ = num;}

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    //如果工作在多线程种，baseloop默认以轮询的方式分配channel给subloop
    EventLoop* getNextLoop();

    //得到线程池中所有的loop
    std::vector<EventLoop*> getAllLoops();

    bool started() const{ return started_;}
    const std::string name(){return name_;}
private:

    EventLoop* baseloop_; //baseloop_是本线程的loop，用户创建的那个loop
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::shared_ptr<EventLoopThread>> threads_; //包含所有创建事件的线程
    std::vector<EventLoop*> loops_; //事件线程里面EventLoop的指针

};