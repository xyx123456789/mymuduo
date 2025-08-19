#include"Buffer.h"

#include <errno.h>
#include <sys/uio.h> // Include the appropriate header file for "iovec" type
#include <unistd.h> // Include the appropriate header file for "write" function

ssize_t Buffer::readFd(int fd,int *savedErrno)
{
    char extrabuf[65536] = {0}; //栈上的空间 64K

    struct iovec vec[2]; //定义两个缓冲区

    //第一块缓冲区
    const size_t writeable = writableBytes();
    vec[0].iov_base = begin()+writeIndex_;
    vec[0].iov_len = writeable;

    //第二块缓冲区
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writeable < sizeof(extrabuf))? 2 : 1;

    //readv能在非连续的缓冲区里面，写入同一个fd上的数据
    const ssize_t n = ::readv(fd,vec,iovcnt);
    if(n<0)
    {
        *savedErrno = errno;
    }
    else if(n<=writeable) //Buffer的可写缓冲区已经足够存储读出来的数据了
    {
        writeIndex_ += n;
    }
    else //writeable不够，王extrabuf里面也写了数据
    {
        writeIndex_ = buffer_.size();
        append(extrabuf,n-writeable); //从writerIndex_开始写n - writable大小的数据
    }

    return n;
}

ssize_t Buffer::writeFd(int fd,int *savedErrno)
{
    ssize_t n = ::write(fd,peak(),readableBytes());
    if(n<0)
        *savedErrno = errno;
    return n;
}