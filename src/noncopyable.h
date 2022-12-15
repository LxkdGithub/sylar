/*
 * @Author: lxk
 * @Date: 2021-12-19 13:58:36
 * @LastEditors: lxk
 * @LastEditTime: 2021-12-29 21:24:05
 */

#ifndef __SPADGER_NONCOPYABLE_H__
#define __SPADGER_NONCOPYABLE_H__

namespace spadger
{

class Noncopyable
{
public:
    Noncopyable() = default;
    ~Noncopyable() = default;

    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
};


}
#endif 