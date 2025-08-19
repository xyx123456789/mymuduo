#pragma once

/*
noncopeable被继承以后，其派生类对象可以正常的构造和析构
但是不可以进行拷贝构造和赋值操作
*/

class noncopeable{
public:
    //拷贝构造跟赋值构造被delete掉，即设置不可拷贝构造和复制，让派生类对象不能拷贝构造和赋值构造
    noncopeable(const noncopeable&) = delete;
    void operator=(const noncopeable&) = delete;

protected:
    noncopeable() = default;
    ~noncopeable() = default;
};