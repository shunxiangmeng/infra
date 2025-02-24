/********************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :   Utils.h
 * Author      :   mengshunxiang 
 * Data        :   2024-02-23 23:13:45
 * Description :   None

 ********************************************************************/
#pragma once
#include <string>
#include <atomic>

namespace infra {
    
//禁止拷贝基类
class noncopyable {
protected:
    noncopyable() {}
    ~noncopyable() {}
private:
    noncopyable(const noncopyable &that) = delete;             //拷贝构造函数
    noncopyable(noncopyable &&that) = delete;                  //移动构造函数
    noncopyable &operator=(const noncopyable &that) = delete;  //拷贝赋值运算符
    noncopyable &operator=(noncopyable &&that) = delete;       //移动赋值运算符
};

std::string exePath(bool isExe = true);
std::string exeDir(bool isExe = true);

template <class C>
class ObjectStatistic {
public:
    ObjectStatistic() {
        ++getCounter();
    }
    ~ObjectStatistic() {
        --getCounter();
    }
    static size_t count() {
        return getCounter().load();
    }
private:
    static std::atomic<size_t>& getCounter();
};

/* 对象个数统计 */
#define ObjectStatisticImpl(Type)  \
    template<> \
    std::atomic<size_t>& infra::ObjectStatistic<Type>::getCounter() { \
        static std::atomic<size_t> instance(0); \
        return instance; \
    }


int32_t getCurrentThreadId();

uint32_t infra_htonl(uint32_t value);
uint16_t infra_htons(uint16_t value);

uint32_t infra_ntohl(uint32_t value);
uint16_t infra_ntohs(uint16_t value);

void setThreadName(const char *name);
std::string getThreadName();

std::string noPathFileName(const std::string path);

#define UPALIGNTO(value, align) ((value + align - 1) & (~(align - 1)))
#define UPALIGNTO2(value) UPALIGNTO(value, 2)
#define UPALIGNTO4(value) UPALIGNTO(value, 4)
#define UPALIGNTO16(value) UPALIGNTO(value, 16)
#define DOWNALIGNTO16(value) (UPALIGNTO(value, 16) - 16)

}
