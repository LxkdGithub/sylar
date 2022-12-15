/*
 * @Author: lxk
 * @Date: 2021-12-09 15:43:58
 * @LastEditors: lxk
 * @LastEditTime: 2021-12-17 20:33:35
 */


#ifndef __SPADGER_SINGLETON_H__
#define __SPADGER_SINGLETON_H__

#include <memory>
namespace spadger 
{


namespace {

template<class T, class X, int N>
T& GetInstanceX() {
    static T v;
    return v;
}

template<class T, class X, int N>
std::shared_ptr<T> GetInstancePtr() {
    static std::shared_ptr<T> v(new T);
    return v;
}
}


template <typename T>
class Singleton 
{
public:
    static T* GetInstance() {
        static T t;
        return &t;
    }
};

template <typename T>
class SingletonPtr
{
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> t(new T);
        return t;
    }
};





}
#endif 