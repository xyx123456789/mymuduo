#pragma once

#include"noncopeable.h"
#include"Socket.h"
#include"Channel.h"

#include<functional>


class EventLoop;
class InetAddress;

class Acceptor
{
public:
    using NewConnectionCallBack = std::function<void(int sockfd,const InetAddress&)>;
    Acceptor(EventLoop* loop,const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallBack& cb)
    {
        newConnectionCallBack_ = cb;
    }

    bool listening() const { return listening_;}
    void listen();
private:
    void handleRead();

    EventLoop* loop_; //Acceptor用的就是用户自定义的那个baseloop，相当于mainloop
    Socket acceptSocket_; //Acceptor的socket
    Channel acceptChannel_; // Acceptor的channel
    
    NewConnectionCallBack newConnectionCallBack_; // 有新连接到来时的回调函数
    bool listening_; // 是否正在监听
};