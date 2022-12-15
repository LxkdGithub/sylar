/*
 * @Author: lxk
 * @Date: 2021-12-25 15:32:29
 * @LastEditors: lxk
 * @LastEditTime: 2021-12-25 15:45:37
 */
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
 
void
myfunc3(void)
{
   int j, nptrs;
#define SIZE 100
   void *buffer[100];
   char **strings;
 
   nptrs = backtrace(buffer, SIZE);
   printf("backtrace() returned %d addresses\n", nptrs);
 
   /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	  would produce similar output to the following: */
    // in backtrace_symbols function char **strings = malloc(sizeof(void*) * nprts); 
    // 然后一个个copy(strings[i], xxxxx);
   strings = backtrace_symbols(buffer, nptrs);
   if (strings == NULL) {
	   perror("backtrace_symbols");
	   exit(EXIT_FAILURE);
   }
 
   for (j = 0; j < nptrs; j++)
	   printf("%s\n", strings[j]);
    
    // 这里不能把strings[i] free掉 应该不是使用malloc的方式分配的内存吧 所以不要担心了
    free(strings);
}
 
// static 函数没有符号 也就是myfunc2这样的名字 只有个地址吧
static void   /* "static" means don't export the symbol... */
myfunc2(void)
{
   myfunc3();
}
 
void
myfunc(int ncalls)
{
   if (ncalls > 1)
	   myfunc(ncalls - 1);
   else
	   myfunc2();
}
 
int
main(int argc, char *argv[])
{
   if (argc != 2) {
	   fprintf(stderr, "%s num-calls\n", argv[0]);
	   exit(EXIT_FAILURE);
   }
 
   myfunc(atoi(argv[1]));
   exit(EXIT_SUCCESS);
}