#include"EPollPoller.h"
#include"Logger.h"
#include"Channel.h"

#include<errno.h>
#include <unistd.h>
#include <string.h>

//channel未添加到poller中
const int kNew = -1; // channel的成员index_ = -1
//channel已经添加到poller中
const int kAdded = 1;
//channel从poller中删除
const int kDeleted = 2;


EPollPoller::EPollPoller(EventLoop* loop)
    :Poller(loop)
    ,epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    ,events_(kInitEventListSize)
 {
    if(epollfd_<0)
    {
        LOG_FATAL("epoll_create error:%d\n",errno);
    }
 }

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeouts,ChannelList* activeChannels)
{
    //实际上应该用LOG_DEBUG输出日志更为合理
    LOG_INFO("func=%s => fd total count:%lu \n",__FUNCTION__,channels_.size());
    //*events_.begin()数组其实元素的数值，然后然后取地址&
    int numEvents = ::epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeouts);

    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if(numEvents>0)
    {
        LOG_FATAL("%d events happend \n",numEvents);
        fillActiveChannels(numEvents,activeChannels);
        //如果满了，就扩容
        if(events_.size() == numEvents)
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0)
        LOG_DEBUG("%s timeout \n",__FUNCTION__);
    else
    {
        if(saveErrno != EINTR) //不等于外部中断
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err");
        }
    }
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel => EPollPoller updateChannel removeChannel
/*
    EventLoop  => poller.poll
    包含Channellist    Poller ，muduo里实现了poll和epll，这里只实现了epoll
                    ChannelMap  <fd,Channel*>
    如果注册过，那么Channlelist会写到ChannelMap里面去
*/


void EPollPoller::updateChannel(Channel*channel)
{
    const int index = channel->index();
    LOG_INFO("func = %s => fd = %d events = %d index = %d \n",__FUNCTION__,channel->fd(),channel->events(),index);

    //要么是新添加要么是被删除的，重新添加
    if(index==kNew || index==kDeleted)
    {
        if(index==kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);
    }
    else //channel已经在poller上注册过了
    {
        int fd = channel->fd();
        if(channel->isNoneEvent()) //对事件没兴趣
        {
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }
        else //对读或者写感兴趣
        {
            update(EPOLL_CTL_MOD,channel);
        }
        LOG_INFO("fd:%d events:%d",fd,channel->events());
    }
}

//从poller中删除channel的逻辑
void EPollPoller::removeChannel(Channel*channel)
{
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s => fd = %d \n",__FUNCTION__,fd);


    int index = channel->index();
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);
}

//填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents,ChannelList *activeChannels) const
{
    for(int i = 0;i<numEvents;i++)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);//EventLoop就拿到了epoller给他返回的所有事件发生的channel列表
    }
}

//更新channel通道 epoll_ctl add/mod/del
void EPollPoller::update(int operation,Channel *channel)
{
    epoll_event event;
    memset(&event,0,sizeof event);
    //bzero(&event.sizeof event);

    int fd = channel->fd();

    event.data.fd = fd;
    event.events = channel->events();
    event.data.ptr = channel;  //ptr指向channel，利用这个可以直接获取之前注册的channel对象进而处理相应事件

    if (::epoll_ctl(epollfd_,operation,fd,&event)<0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n",errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n",errno);
        }
    }
}