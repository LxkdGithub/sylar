/*
 * @Author: lxk
 * @Date: 2021-12-19 14:58:17
 * @LastEditors: lxk
 * @LastEditTime: 2021-12-19 15:21:31
 */
#include "../src/mutex.h"
// #include <iostream>

using namespace std;


int main() {
    spadger::Mutex mutex;
    spadger::Mutex::Lock lock(mutex);
}
