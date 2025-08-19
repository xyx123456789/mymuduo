#pragma once

#include"noncopeable.h"
#include"Timestamp.h"
#include"CurrentThread.h"

#include<vector>
#include<atomic>
#include<functional>
#include<memory>
#include<mutex>

class Channel;
class Poller;

//事件循环类   主要包含两大模块 Channel  Poller(epoll的抽象)
class EventLoop:noncopeable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();//事件循环
    void quit();//退出事件循环

    Timestamp pollReturnTime() const {return pollReturnTime_;}

    void runInloop(Functor cb);//在当前loop线程执行cb
    void queueInloop(Functor cb);//将cb放入队列中，唤醒loop所在线程执行cb

    void wakeup();//mainreactor唤醒subreactor，唤醒loop所在的线程

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

    bool hasChannel(Channel* channel);
    //判断当前的EventLoop对象是否处于创建它所在的线程
    bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}

private:
    void handleRead(); //wakeupChannel_的读回调
    void doPendingFunctors(); //执行回调操作

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;//原子操作,通过CAS实现,判断是否正在进行loop循环
    std::atomic_bool quit_;//原子操作,通过CAS实现,退出loop循环
    const pid_t threadId_;//记录当前loop所在的线程id
    Timestamp pollReturnTime_;//poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    //当mainloop获取一个新用户的channel，通过轮询算法分配一个subloop，通过该成员唤醒subloop处理channel
    int wakeupFd_;//用于唤醒poller的fd,主要用于跨线程唤醒，轮询线程与业务线程之间的通信
    std::unique_ptr<Channel> wakeupChannel_;//wakeupChannel_是一个channel对象，用于处理wakeupFd_上的事件

    ChannelList activeChannels_;//当前poller返回的活跃的channel列表
    Channel* currentactiveChannel_;//当前正在处理的channel

    std::atomic_bool callingPendingFunctors_;//原子操作,通过CAS实现,标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;//存放loop所有需要执行的回调操作
    std::mutex mutex_;//互斥锁，用于保护pendingFunctors_的操作
};