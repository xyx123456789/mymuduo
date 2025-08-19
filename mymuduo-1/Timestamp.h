#pragma once

#include<iostream>
#include<string>

class Timestamp{
public:
        Timestamp();
        explicit Timestamp(int64_t microSecondsSinceEpoch);  //explicit关键字防止隐式转换，更好的控制代码的行为

        //获取当前时间
        static Timestamp now();
        //将时间输出
        std::string toString() const;

private:
    int64_t microSecondsSinceEpoch_;

};