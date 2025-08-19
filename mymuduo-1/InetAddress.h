#pragma once

#include<string>
#include<arpa/inet.h>
#include<netinet/in.h>

//封装socket地址类型
class InetAddress{
public:
    explicit InetAddress(uint64_t port = 0,std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr):
        addr_(addr)
    {}

    //打印ip
    std::string toIp() const;
    //打印ip和数据
    std::string toIpPort() const;
    //打印端口
    uint64_t toPort() const;

    //函数返回值是const类型。这意味着你不能修改函数的返回值。这通常用于返回一个对象的引用或指针，你不希望调用者修改这个对象。
    const sockaddr_in* getSockAddr() const {return &addr_;}
    void setSockAddr(const sockaddr_in &addr) {addr_ = addr;}

private:
    sockaddr_in addr_;
};