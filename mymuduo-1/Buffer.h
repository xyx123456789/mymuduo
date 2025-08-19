#pragma once

#include<vector>
#include<string>
#include<algorithm>

//网络库底层的缓冲器类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8; //预留8字节的空间,数据包的大小
    static const size_t kInitialSize = 1024; // buffer初始化大小
    
    //可读区域长度
    size_t readableBytes() const 
    {
        return writeIndex_-readerIndex_;
    }

    //可写区域长度
    size_t writableBytes() const
    {
        return buffer_.size()-writeIndex_;
    }

    // 返回前面空闲的区域
    size_t prependableBytes() const 
    {
        return readerIndex_;
    }

    // buffer_.size - writerIndex_  len
    void ensureWritableBytes(size_t len)
    {
        if(writableBytes() < len)
            makeSpace(len); //扩容函数
    }

    //复位操作
    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            readerIndex_ += len; //应用只读取了可读缓冲区的一部分
        }
        else
        {
             //如果读完了，重新复位
             retrieveAll();
        }
    }

    //把onMessage函数上报的buffer数据转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string res(peak(),len);
        retrieve(len); //把读过的数据清空,读走了，那么readerIndex_也要往后移动
        return res;
    }

    void retrieveAll()
    {
        //重新复位
        readerIndex_ = writeIndex_ = kCheapPrepend;
    }

    char *beginWrite()
    {
        return begin() + writeIndex_;
    }

    const char *beginWrite() const
    {
        return begin() + writeIndex_;
    }

    //把[data,data+len]内存上数据添加到writeable区域
    void append(const char* data,size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data,data+len,beginWrite());
        writeIndex_+=len;
    }

    //从fd上读数据
    ssize_t readFd(int fd,int *savedErrno);
    //从fd上写数据
    ssize_t writeFd(int fd,int *savedErrno);


    //返回缓冲区中可读数据的起始地址
    const char* peak() const
    {
        return begin()+readerIndex_;
    }
private:
    char* begin()
    {
        return &*buffer_.begin();
    }

    const char* begin() const
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {
        //如果可写区域加上前面空闲区域小于len+预留，扩容
        if(writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writeIndex_+ len);
        }
        else
        {
            //否则，将可读区域前移到预留区域
            //template <class InputIterator, class OutputIterator>
            //OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result);
            //first: 指向要复制的第一个元素的输入迭代器。
            //last: 指向要复制的最后一个元素之后的输入迭代器。
            //result: 指向目标范围起始位置的输出迭代器。
            size_t readable = readableBytes();
            std::copy(begin()+ readerIndex_,begin()+writeIndex_,begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writeIndex_ = readerIndex_+readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_; //需要读，就从读索引开始往后读
    size_t writeIndex_; //需要写，就从写索引开始往后写
};