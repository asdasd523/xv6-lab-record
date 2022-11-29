#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

//gdb-multiarch

int
main(int argc, char *argv[])
{
  int i;
  char *nargv[MAXARG];

  if(argc < 3 || (argv[1][0] < '0' || argv[1][0] > '9')){  //检查第一位数字
    fprintf(2, "Usage: %s mask command\n", argv[0]);
    exit(1);
  }

  if(trace(atoi(argv[1])) < 0) {
    fprintf(2, "%s: trace failed\n", argv[0]);
    exit(1);
  }
  
  for(i = 2; i < argc && i < MAXARG; i++){
    nargv[i-2] = argv[i];
  }
  exec(nargv[0], nargv);
  exit(0);
}



/*
实验过程：
1.在PCB即proc.h中的结构体proc，并且添加mask变量，以指示需要追踪的系统调用
2.在syscall中打印各个信息
3.最外层,bash trace执行后，在内核PCB中添加对应系统调用mask，之后在每次系统调用时，
都会在syscall处调用打印信息
*/