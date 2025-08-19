#pragma once

#include"noncopeable.h"

#include<functional>
#include<memory>
#include<thread>
#include<unistd.h>
#include<string>
#include<atomic>

class Thread:noncopeable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func,const std::string &name = std::string());
    ~Thread();

    void start();
    void join(); //等待线程结束

    bool started() const { return started_;}
    pid_t tid() const { return tid_;}
    const std::string &name() const { return name_;}
    static int numCreated(){ return numCreated_;}
private:
    void setDefaultName();

    bool started_;
    bool joined_;
    //使用thread库，不再需要c的pthread库
    // pthread_t pthreadId_;
    //使用智能指针管理对象可以控制线程开始的时机，如果直接用thread定义，那么对应绑定的线程函数会直接启动
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;//线程id
    ThreadFunc func_; //存放线程函数
    std::string name_; //线程名字
    static std::atomic_int numCreated_;//原子操作，防止多线程操作时出现问题，保存的是创建的线程数
    
};