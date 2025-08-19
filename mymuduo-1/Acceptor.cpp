#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <error.h>
#include <unistd.h>

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET,SOCK_CLOEXEC | SOCK_STREAM | SOCK_NONBLOCK ,IPPROTO_TCP);
    if(sockfd<0)
    {
        LOG_FATAL("%s:%s:%d,sockets::createNonblockingOrDie:%d\n",__FILE__,__FUNCTION__,__LINE__,errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking())
    , acceptChannel_(loop,acceptSocket_.fd())
    , listening_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);
    //TcpServer::start() Acceptor.listen 有新用户连接，要执行一个回调 （confd => channel => subloop)
    //baseloop => acceptChannel_(listenfd) => acceptChannel_
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disenableAll(); //Acceptor::acceptChannel_不关注任何事件
    acceptChannel_.remove(); //Acceptor::acceptChannel_从Poller中移除
}


void Acceptor::listen()
{
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

//listenfd有事件发生了，也就是有新用户连接了
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd>=0)
    {
        if(newConnectionCallBack_)
            newConnectionCallBack_(connfd,peerAddr); //轮询找到subloop，唤醒分发当前的新客户端的Channel
        else
            ::close(connfd);
    }
    else
    {
        LOG_ERROR("%s:%s:%d,Acceptor::heandleRead:%d\n",__FILE__,__FUNCTION__,__LINE__,errno);
    }
}