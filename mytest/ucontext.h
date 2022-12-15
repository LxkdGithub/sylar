/*
 * @Author: lxk
 * @Date: 2021-12-23 13:51:30
 * @LastEditors: lxk
 * @LastEditTime: 2021-12-23 20:46:02
 */

#ifndef __SPADGER_FIBER_H__
#define __SPADGER_FIBER_H__

#include <functional>
#include <memory>
#include <ucontext.h> 
#include <map>
#include <stdint.h>
#include <string.h>
#include "singleton.h"


const uint64_t DEFAULT_STACK_SIZE = 1024 * 1024;

enum CoState {FREE=0, RUNNING=1, SUSPEND=2};

class Coroutine 
{
public:
    Coroutine();
    virtual ~Coroutine();

    virtual void CoProcess();

    void Resume();  // 恢复

    uint32_t GetId() const { return m_id; };

    CoState GetState() const { return m_state; };

    void SetId(uint32_t id) { m_id = id; }

    void SetState(CoState state) { m_state = state; }

protected:
    // 挂起
    void Yield();

    // 保存堆栈
    void SaveStack(); 

    void ReloadStack();

public:
    char *m_buffer;
    ucontext_t m_ctx;

private:
    uint32_t m_stack_size;
    uint32_t m_cap;   // ????
    uint32_t m_id;
    CoState m_state;
};

class Scheduler 
{
public:
    typedef std::map<uint32_t, Coroutine*> CrtMap;
    Scheduler(); 
    virtual ~Scheduler();
    // 初始化协程
    void CoroutineNew(Coroutine* crt);
    
    // 指定用户协程入口函数 ???
    static void CoroutineEntry(void* crt); 

    // 协程恢复函数
    void Resume(uint32_t id);
    
    // 检查和清理协程池
    int HasCoroutine(); 

    // remove
    void Remove(uint32_t id);

    char* GetStackBottom() {
        return stack + DEFAULT_STACK_SIZE;
    }
public:
    ucontext_t main_ctx;
    char stack[DEFAULT_STACK_SIZE];
private:
    CrtMap crtPool;
};


typedef spadger::Singleton<Scheduler> SingleSchedule;

#endif 