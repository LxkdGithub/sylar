/*
 * @Author: lxk
 * @Date: 2021-12-29 10:57:20
 * @LastEditors: lxk
 * @LastEditTime: 2021-12-29 12:19:34
 */
// #include <ucontext.h>
// #include <iostream>
// #include <unistd.h>
// #include <iomanip>



// int main() {
//     ucontext_t context;
//     getcontext(&context);
//     std::cout << std::hex << context.uc_mcontext.gregs[0];
//     puts("hello");
//     sleep(1);
//     setcontext(&context);
//     return 0;
// }

#include <ucontext.h>
#include <iostream>
#include <unistd.h>

void newContextFun() {
  std::cout << "this is the new context" << std::endl;
}


int main() {
  char stack[10*1204];
  //get current context
  ucontext_t curContext;
  getcontext(&curContext);
  //modify the current context
  ucontext_t newContext = curContext;
  newContext.uc_stack.ss_sp = stack;
  newContext.uc_stack.ss_size = sizeof(stack);
  newContext.uc_stack.ss_flags = 0;
  newContext.uc_link = &curContext;
  //register the new context
  makecontext(&newContext, (void(*)(void))newContextFun, 0);
  swapcontext(&curContext, &newContext);
  printf("main\n");
  return 0;
}