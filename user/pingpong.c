// user/pingpong.c

#include "kernel/types.h"
#include "user/user.h"

int
main()
{
  int p1[2];  // pipe: parent -> child
  int p2[2];  // pipe: child -> parent

  pipe(p1);
  pipe(p2);

  int pid = fork();
  if(pid < 0){
    fprintf(2, "fork failed\n");
    exit(1);
  }

  if(pid == 0){
    // 子进程
    char buf;
    read(p1[0], &buf, 1);  // 从父进程读取
    printf("%d: received ping\n", getpid());
    write(p2[1], &buf, 1); // 写给父进程
    exit(0);
  } else {
    // 父进程
    char ch = 'X';
    write(p1[1], &ch, 1);  // 发送给子进程
    wait(0);               // 等待子进程
    read(p2[0], &ch, 1);   // 从子进程读取
    printf("%d: received pong\n", getpid());
    exit(0);
  }
}
