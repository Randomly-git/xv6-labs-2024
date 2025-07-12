#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
  char buf[512];           // 用于读取整行输入
  char *args[MAXARG];      // 命令参数
  int i;

  // 初始化前缀命令参数，如 xargs echo bye => ["echo", "bye"]
  for(i = 1; i < argc; i++){
    args[i - 1] = argv[i];
  }

  int n = i - 1; // 原始参数个数

  int idx = 0;
  char ch;
  while(read(0, &ch, 1) == 1){
    if(ch == '\n'){
      buf[idx] = 0; // 行结束，手动加上 '\0'

      // 处理当前输入行，将其按空格拆分成多个参数
      int j = 0;
      char *p = buf;
      while(*p){
        // 跳过前导空格
        while(*p && *p == ' ')
          p++;
        if(*p == 0) break;
        args[n + j] = p;
        j++;
        // 移动到下一个空格并替换为空字符
        while(*p && *p != ' ')
          p++;
        if(*p){
          *p = 0;
          p++;
        }
      }
      args[n + j] = 0; // exec 参数需以 NULL 结尾

      // 创建子进程执行命令
      if(fork() == 0){
        exec(args[0], args);
        fprintf(2, "exec failed\n");
        exit(1);
      } else {
        wait(0);
      }

      // 准备读下一行
      idx = 0;
    } else {
      if(idx < sizeof(buf) - 1)
        buf[idx++] = ch;
    }
  }

  exit(0);
}
