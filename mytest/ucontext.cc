/*
 * @Author: lxk
 * @Date: 2021-12-23 15:24:12
 * @LastEditors: lxk
 * @LastEditTime: 2021-12-23 21:49:07
 */


#include "ucontext.h"
#include <assert.h>
#include <iostream>

Coroutine::Coroutine()
    :m_id(0), m_state(FREE), m_cap(0), m_stack_size(0),m_buffer(nullptr)
{

}

Coroutine::~Coroutine() 
{
    delete []m_buffer;
}

void Coroutine::CoProcess() {

}


void Coroutine::Resume()
{
    if(m_state == SUSPEND) {
        ReloadStack();
        m_state = RUNNING;
        swapcontext(&(SingleSchedule::GetInstance()->main_ctx), &m_ctx);
    }
}

void Coroutine::Yield() {
    if(m_state == RUNNING) {
        SaveStack();
        m_state = SUSPEND;
        swapcontext(&m_ctx, &(SingleSchedule::GetInstance()->main_ctx));
    }
}


void Coroutine::SaveStack() 
{
    // 栈底部
    char * stackBottom = SingleSchedule::GetInstance()->GetStackBottom();
    char dumy = 0;   // dumy的作用是确定顶部用到了哪里

    assert(stackBottom - &dumy <= DEFAULT_STACK_SIZE);

    if(m_cap < stackBottom-&dumy) {
        if(m_buffer) {
            delete []m_buffer;
        }
        m_cap = stackBottom - &dumy;
        m_buffer = new char[m_cap]; 
    }
    m_stack_size = stackBottom - &dumy;
    memcpy(m_buffer, &dumy, m_stack_size);
}

void Coroutine::ReloadStack() 
{
    memcpy(SingleSchedule::GetInstance()->GetStackBottom()-m_stack_size, m_buffer, m_stack_size);
}


//----------------------------------------------------------------------

Scheduler::Scheduler() 
{

}

Scheduler::~Scheduler()
{

}

void Scheduler::CoroutineEntry(void* crt) {
    ((Coroutine*)crt)->SetState(RUNNING);
    ((Coroutine*)crt)->CoProcess();
    ((Coroutine*)crt)->SetState(FREE);
}

void Scheduler::CoroutineNew(Coroutine* crt) {
    uint32_t id = crt->GetId();
    CoState state = CoState(crt->GetState());
    assert(id != 0);
    assert(state == FREE);

    if(crtPool[id] != nullptr) {
        auto it = crtPool.find(id);
        crtPool.erase(it);
    }

    getcontext(&(crt->m_ctx));
    crt->m_ctx.uc_stack.ss_sp = stack;      // 这里为每一个协程设置运行的参数 
                                            // 包括栈的地址 这里需要系统运行栈替换 所以需要使用库的设计
    crt->m_ctx.uc_stack.ss_size = DEFAULT_STACK_SIZE;
    crt->m_ctx.uc_stack.ss_flags = 0;
    crt->m_ctx.uc_link = &main_ctx;     // 执行完后回主协程
    crtPool[id] = crt;

    makecontext(&crt->m_ctx, (void(*)(void))CoroutineEntry, 1, (void*)crt);
    swapcontext(&main_ctx, &crt->m_ctx);
}

void Scheduler::Resume(uint32_t id) {
    if(crtPool[id] != nullptr) {
        crtPool[id]->Resume();
    }
}

int Scheduler::HasCoroutine() {
    int count = 0;
    CrtMap::iterator it;
    for(it = crtPool.begin(); it != crtPool.end(); it++) {
        if(it->second->GetState() != FREE) {
            count++;
        } else {
            it = crtPool.erase(it);
            it--; // erase后it自动指向下一个
        }
    }
    return count;
}


void Scheduler::Remove(uint32_t id) {
    if(crtPool[id] != nullptr) {
        crtPool.erase(crtPool.find(id));    // 必须得到iterator才可以erase
    }
}

// RNN 安全态势感知
// 群体免疫技术的容错技术

// GAN
// 强化学习

