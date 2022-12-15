/*
 * @Author: lxk
 * @Date: 2021-12-28 20:51:48
 * @LastEditors: lxk
 * @LastEditTime: 2021-12-28 20:59:44
 */

#include <iostream>
#include <thread>
#include <vector>

static thread_local int hello = 10;

void func() {
    {
        std::cout << hello << std::endl;
    }
    hello = 100;
}

int main() {
    std::vector<std::thread*> thrs;
    hello = 9;             
    for(int i=0;i<3;++i) {
        thrs.push_back(new std::thread(func));
    }
    for(int i=0;i<3;++i) {
        thrs[i]->join();
    }

    std::cout << "main end......" << std::endl;
}