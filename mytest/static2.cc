/*
 * @Author: lxk
 * @Date: 2021-12-29 14:37:21
 * @LastEditors: lxk
 * @LastEditTime: 2021-12-29 14:39:22
 */
#include <iostream>

static int hello = 100;
void func();

int main() {
    std::cout << "main " << hello << std::endl;
    func();

}