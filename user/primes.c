#include "kernel/types.h"
#include "user/user.h"

int out_fd;  // 主进程打印用的管道写端，全局传入各层

void primes(int p_read) __attribute__((noreturn));

void primes(int p_read) {
  int prime, num;
  int p[2];

  if (read(p_read, &prime, sizeof(prime)) == 0) {
    close(p_read);
    exit(0);
  }

  // 把 prime 写给主进程统一打印
  write(out_fd, &prime, sizeof(prime));

  pipe(p);

  if (fork() == 0) {
    // 子进程处理后续数
    close(p[1]);         // 关闭写端
    close(p_read);       // 不用旧输入
    primes(p[0]);        // 递归调用
  } else {
    // 父进程过滤数字后写给子进程
    close(p[0]);         // 关闭读端
    while (read(p_read, &num, sizeof(num)) != 0) {
      if (num % prime != 0) {
        write(p[1], &num, sizeof(num));
      }
    }
    close(p_read);
    close(p[1]);
    wait(0);             // 等待子进程
  }

  exit(0);
}

int main() {
  int p[2];         // 初始筛选管道
  int out_pipe[2];  // 主进程专用输出管道

  pipe(p);
  pipe(out_pipe);
  out_fd = out_pipe[1];  // 设置为全局变量

  if (fork() == 0) {
    // primes 分支
    close(p[1]);           // 关闭写端
    close(out_pipe[0]);    // 不读输出
    primes(p[0]);
  } else {
    // 主进程写入 2~280
    close(p[0]);           // 不读输入
    for (int i = 2; i <= 280; i++) {
      write(p[1], &i, sizeof(i));
    }
    close(p[1]);           // 写完关闭

    // 主进程读取 prime 输出
    close(out_pipe[1]);    // 关闭写端（只读）
    int prime;
    while (read(out_pipe[0], &prime, sizeof(prime)) > 0) {
      printf("prime %d\n", prime);
    }

    close(out_pipe[0]);
    while (wait(0) != -1); // 等待所有子进程
  }

  exit(0);
}

