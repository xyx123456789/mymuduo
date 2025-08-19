#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<strings.h>
#include <netinet/tcp.h>  /* for TCP_XXX defines */

Socket::~Socket()
{
    close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if(0!=bind(sockfd_,(sockaddr*)localaddr.getSockAddr(),sizeof(sockaddr_in)))
    {
        LOG_FATAL("Socket::bindAddress %d fail \n",sockfd_);
    }
}
void Socket::listen()
{
    if(0!=::listen(sockfd_,1024))
    {
        LOG_FATAL("Socket::listen %d fail \n",sockfd_);
    }
}

void Socket::shutdownWrite(){
    if(::shutdown(sockfd_,SHUT_WR) < 0) 
    {
        LOG_ERROR("Socket::shutdownWrite %d fail \n",sockfd_);
    }
}


int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    bzero(&addr,sizeof addr);
    socklen_t len = sizeof(addr);
    int connfd = ::accept4(sockfd_,(sockaddr*)&addr,&len,SOCK_NONBLOCK | SOCK_CLOEXEC);  //SOCK_CLOEXEC 是一个用于 accept4 系统调用的标志，它的作用是在执行 exec 系列函数（如 execve）时自动关闭文件描述符
    if (connfd>=0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::setTcpNoDelay(bool on)  //设置TCP_NODELAY选项，禁用Nagle算法，不缓冲数据
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof optval);
}
void Socket::setReuseAddr(bool on)   //设置SO_REUSEADDR选项，允许重用本地地址和端口
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof optval);
}
void Socket::setReusePort(bool on)  //设置SO_REUSEPORT选项，允许多个进程或线程绑定同一个口号
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof optval);
}
void Socket::setKeepAlive(bool on)   //设置SO_KEEPALIVE选项，保活机制，探测空闲连接
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof optval);
}