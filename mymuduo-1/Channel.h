#pragma once

#include"noncopeable.h"
#include"Timestamp.h"
#include<functional>
#include<memory>

class EventLoop;



/*
Channel 理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN,EPOLLOUT事件
还绑定了poller返回的具体事件
*/
class Channel{
public:
    //EventCallback类型别名：
    //定义了一个不接受参数且不返回任何值的函数类型。这意味着任何符合这一签名的函数、Lambda表达式或可调用对象都可以被EventCallback类型的变量所持有和调用。这在事件处理中非常有用，因为它允许将不同的回调函数绑定到特定事件上，而不需要关心这些回调函数的具体实现。
    //ReadEventCallback类型别名：
    //定义了一个接受Timestamp类型参数且不返回任何值的函数类型。这允许将需要时间戳信息的回调函数绑定到特定事件上。例如，在处理某些读事件时，可能需要知道事件发生的确切时间，这时就可以使用ReadEventCallback类型的变量来持有这样的回调函数。
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop,int fd);
    ~Channel();

    // fd得到poller通知以后，处理事件的调用相应的回调方法
    void handleEvent(Timestamp receiveTime);

    //设置回调函数对象
    void setReadCallback(ReadEventCallback cb){readCallback_ = std::move(cb);}
    void setWriteCallback(EventCallback cb){writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb){closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb){errorCallback_ = std::move(cb);}

    //防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&obj);

    int fd() const {return fd_;}
    int events() const {return events_;}
    //设置在fd上真正发生的事件
    int set_revents(int revt) {revents_ = revt;return revt;}

    //设置fd相应的事件状态
    void enableReading(){events_ |= kReadEvent;update();}
    void disenableReading(){events_ &= ~kReadEvent;update();}
    void enableWriting(){events_ |= kWriteEvent;update();}
    void disenableWriting(){events_ &= ~kWriteEvent;update();}
    void disenableAll(){events_ = kNoneEvent;update();}

    //返回fd当前事件的对应状态
    bool isNoneEvent(){return events_ == kNoneEvent;}
    bool isWriting(){return events_ & kWriteEvent;}
    bool isReading(){return events_ & kReadEvent;}

    int index(){return index_;}
    void set_index(int index){index_ = index;}

    EventLoop* owerLoop(){return loop_;}
    //在channel所属的EventLoop中，把当前的channel删除掉
    void remove();// 删除channel


private:

    //当改变Channel所表示fd的事件events后，update负责在poller里面更改fd相应的事件epoll_ctl
    void update();
    //受保护的处理事件
    void handleEventWithGuard(Timestamp receiveTime);

    //对不同事件感兴趣的回调函数
    static const int kNoneEvent; //对没有事件感兴趣
    static const int kReadEvent; //对读事件感兴趣
    static const int kWriteEvent; //对写事件感兴趣

    EventLoop* loop_; //事件循环
    const int fd_; //fd,poller监听的对象
    int events_; //注册fd感兴趣的事件
    int revents_; //poller返回的具体发生的事件
    int index_;

    //弱指针监听channel是否被remove
    std::weak_ptr<void> tie_;
    
    bool tied_;

    //因为channel通道里面能够获知fd最终发生的具体事件revents,所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

};