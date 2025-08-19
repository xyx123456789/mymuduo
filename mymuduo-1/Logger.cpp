#include"Logger.h"
#include"Timestamp.h"
#include<iostream>

 //获取日志唯一实例对象
Logger& Logger::Instance()
{
    static Logger log;
    return log;
}

//设置日志级别
 void Logger::setLogLevel(int Level)
 {
    logLevel_ = Level;
 }

//写日志
void Logger::log(std::string msg)
{
    switch (logLevel_)
    {
    case INFO:
        std::cout<<"[INFO]";
        break;
    case ERROR:
        std::cout<<"[ERROR]";
        break;
    case FATAL:
        std::cout<<"[FATAL]";
        break;
    case DEBUG:
        std::cout<<"[DEBUG]";
        break;
    default:
        break;
    }

    //打印时间和msg
    std::cout<<Timestamp::now().toString()<<":"<<msg<<std::endl;
}