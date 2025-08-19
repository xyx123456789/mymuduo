#include "EventLoop.h"
#include "Logger.h"
#include "Channel.h"
#include "Poller.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory>
#include <errno.h>

// 防止一个线程创建多个EventLoop
__thread EventLoop *t_loopInThisThread_ = nullptr;

// 定义默认Poller IO复用超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd,用来notify唤醒subReator处理新来的channel
int createEventfd()
{
    // 创建一个eventfd,用来在IO线程之间传递消息，非阻塞的且线程安全的
    int evfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evfd < 0)
        LOG_FATAL("eventfd error : %d\n", errno);
    return evfd;
}

EventLoop::EventLoop()
    : looping_(false) // 没有开启循环
      ,
      quit_(false) // 没有退出
      ,
      callingPendingFunctors_(false) // 没有需要执行的回调操作
      ,
      threadId_(CurrentThread::tid()) // 当前loop所在线程的tid
      ,
      poller_(Poller::newDefaultPoller(this)), wakeupFd_(createEventfd()) // 创建wakeupfd
      ,
      wakeupChannel_(new Channel(this, wakeupFd_)) // 创建wakeupChannel
{
    LOG_DEBUG("EventLoop created &p in thread %d \n", this, threadId_);

    // 判断当前线程是否创建了EventLoop对象
    if (t_loopInThisThread_)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread_, threadId_);
    }
    else
    {
        t_loopInThisThread_ = this;
    }

    // 设置wakeupChannel的事件类型为读事件，回调函数为handleRead
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每个EventLoop对象都关注wakeupChannel的EPOLLIN读事件
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    // wakeupChannel_不关注任何事件
    wakeupChannel_->disenableAll();

    // wakeupChannel_从Poller中移除
    wakeupChannel_->remove();

    // 关闭wakeupfd
    ::close(wakeupFd_);
    t_loopInThisThread_ = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (!quit_)
    {
        activeChannels_.clear();
        // 调用Poller的poll函数，等待事件发生
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

        for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些channel发生事件了，然后上报给Eventloop，Eventloop再调用channel的handleEvent函数
            // 遍历activeChannels_，调用每个channel的handleEvent函数
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        /*
         * 1.处理当前EventLoop需要处理的回调操作
         * 2.处理完回调操作后，执行doPendingFunctors()函数
         * IO线程内部，一般不会阻塞，只有在执行回调操作时，用户有可能会阻塞
         * IO线程 mainloop accept fd 《= channel subloop
         * mainloop事先注册一个回调（需要sublooop执行）
         */
        doPendingFunctors(); // 具体的去调用执行回调的时候
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false; // 退出循环
}

// 退出事件循环，1.loop在自己线程中调用quit，2.在其他线程中调用quit
void EventLoop::quit()
{
    quit_ = true;

    if (!isInLoopThread()) // 如果是在其他线程中调用quit：例如在subloop（woker）中调用mainloop的quit
    {
        wakeup(); // 先唤醒poller，才能进行quit
    }
}

void EventLoop::runInloop(Functor cb) // 在当前loop线程执行cb
{
    if (isInLoopThread())
    {
        cb(); // 在当前的loop线程中，执行cb
    }
    else
    {
        queueInloop(cb); // 在非当前loop线程执行cb，就需要唤醒loop线程，执行cb
    }
}

void EventLoop::queueInloop(Functor cb) // 将cb放入队列中，唤醒loop所在线程执行cb
{
    {
        // 有可能多个线程同时调用一个线程的回调，线程安全加锁
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的、需要执行上面回调操作的loop线程了
    // callingPendingFunctors_为true，说明当前loop已经有需要执行的回调操作了，即正在执行回调，又写了新的回调上来
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

// 每一个sub reactor监听了一个wake up channel，main reactor可以通过给wake up channel发送一个write消息，sub reactor就能感知到wakeupfd_有读事件发生
void EventLoop::handleRead()
{
    uint64_t one = 1;                              // 64位无符号整数。用来接受wakeupfd的数据
    ssize_t n = read(wakeupFd_, &one, sizeof one); // 用来读取wakeupFd
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8 \n", n);
    }
}

void EventLoop::wakeup() // mainreactor唤醒subreactor，唤醒loop所在的线程
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_,&one,sizeof one);
    if(n!=sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors() // 执行回调操作
{
    //建立一个局部对象，将pendingFunctors_中的所有回调函数放到functors中
    //防止当前loop执行pendingFunctors_里面回调函数过多时，导致mainloop无法及时向pendingFunctors_注册回调函数，导致阻塞

    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor &functor : functors)
    {
        functor();
    }

    callingPendingFunctors_ = false;
}