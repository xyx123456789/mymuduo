#pragma once

#include<unistd.h>
#include<sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_cachedTid; //声明线程id,，，每个线程都有一份，并且其他的线程看不到

    void cacheTid();

    inline int tid()
    {
        if(__builtin_expect(t_cachedTid == 0,0))
            cacheTid();
        return t_cachedTid;
    }
}