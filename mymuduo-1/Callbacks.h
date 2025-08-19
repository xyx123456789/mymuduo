#pragma once

#include<memory>
#include<functional>

class Buffer;
class TcpConnection;
class Timestamp;

//指向了TcpConnection对象的智能指针
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&,Buffer*,Timestamp)>;


//客户端发送的数据过快，一旦服务器接受数据过慢，就会导致数据丢失，这个回调是防止这类事情发生
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&,size_t)>;