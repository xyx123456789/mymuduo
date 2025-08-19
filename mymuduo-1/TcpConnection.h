#pragma once

#include"noncopeable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

/*
TcpServer通过Acceptor,有一个新用户连接，通过accept拿到connfd

打包TcpConnection,然后设置回调，再把回调设置到对应的Channel里，并注册到Poller中
当Poller中有事件发生，就调用Channel的回调

*/
class TcpConnection : noncopeable,public std::enable_shared_from_this<TcpConnection> //得到当前对象的智能指针
{
public:
    TcpConnection(EventLoop* loop,
                  const std::string &name,
                  int sockfd,
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_;}
    const std::string& name() const { return name_;}
    const InetAddress& localAddress() const { return localAddr_;}
    const InetAddress& peerAddress() const { return peerAddr_;}

    bool connected() const { return state_ == kConnected;}

    //发送数据
    void send(const std::string& buf);
    //关闭连接
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb,size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }
    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }  

    //建立连接
    void connectEstablished();
    //销毁连接
    void connectDestroyed();
private:
    //已经断开，正在断开，已经连接，正在连接
    enum StateE{kDisconnceted,kDisconnceting,kConnected,kConnecting};
    void setState(StateE s) { state_ = s; }
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    
    void sendInLoop(const void* data,size_t len);    
    void shutdownInLoop();

    EventLoop *loop_; //这里不是baseloop_,因为TcpConnection是在subloop中管理的
    const std::string name_;
    std::atomic_int state_; //原子操作，用于保证线程安全
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_; //当前主机ip端口
    const InetAddress peerAddr_; //客户端ip端口

    ConnectionCallback connectionCallback_;  //新连接建立时的回调函数
    MessageCallback messageCallback_;  //消息到来时的回调函数
    WriteCompleteCallback writeCompleteCallback_;  //消息发送完毕时的回调函数
    CloseCallback closeCallback_;  //连接关闭时的回调函数
    HighWaterMarkCallback highWaterMarkCallback_; //高水位回调函数

    size_t highWaterMark_; //水位线标志

    Buffer inputBuffer_; //接受数据的缓冲区
    Buffer outputBuffer_; //发送数据的缓冲区
};