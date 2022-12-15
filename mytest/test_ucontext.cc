/*
 * @Author: lxk
 * @Date: 2021-12-23 20:40:48
 * @LastEditors: lxk
 * @LastEditTime: 2021-12-23 21:49:02
 */
#include <stdio.h>
#include <iostream>
#include <memory>
#include "ucontext.h"

class Logic1 : public Coroutine{
    void CoProcess(){
        puts("1");
        Yield();
        puts("4");
        Yield();
        puts("7");
    }
};

class Logic2 : public Coroutine{
    void CoProcess(){
        puts("2");
        Yield();
        puts("5");
        Yield();
        puts("8");
    }
};

class Logic3 : public Coroutine{
    void CoProcess(){
        puts("3");
        Yield();
        puts("6");
        Yield();
        puts("9");
    }
};

int main() {

    std::shared_ptr<Coroutine> ct1(new Logic1());
    std::shared_ptr<Coroutine> ct2(new Logic2());
    std::shared_ptr<Coroutine> ct3(new Logic3());

    ct1->SetId(1);
    ct2->SetId(2);
    ct3->SetId(3);

    // 其实每一行创建之后都开始运行了 
    // swapcontext 其实就是替换寄存器这些(最重要是的指令寄存器和段寄存器)
    
    SingleSchedule::GetInstance()->CoroutineNew(ct1.get());
    SingleSchedule::GetInstance()->CoroutineNew(ct2.get());
    SingleSchedule::GetInstance()->CoroutineNew(ct3.get());
    printf("Oh! no!!!\n");


    SingleSchedule::GetInstance()->Resume(1);
    SingleSchedule::GetInstance()->Resume(2);
    SingleSchedule::GetInstance()->Resume(3);
    SingleSchedule::GetInstance()->Resume(1);
    SingleSchedule::GetInstance()->Resume(2);
    SingleSchedule::GetInstance()->Resume(3);


    // SingleSchedule::GetInstance()->Remove(1);
    // SingleSchedule::GetInstance()->Remove(2);
    // SingleSchedule::GetInstance()->Remove(3);

    int count = SingleSchedule::GetInstance()->HasCoroutine();
    printf("%d\n", count);

    return 0;
}