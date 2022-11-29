
#include "kernel/types.h"
#include "user/user.h"


//ping pong 实验
/***
1.管道的描述符在父子进程之间共享，某个进程在使用管道时，使用读端就要关闭写端，反之亦然。
2.反复运行test，为什么进程pid是在不断累加，是因为之前创建的进程无法释放?
***/
// int
// main()
// {
//     int p1[2],p2[2];
//     // char *argv[2];
//     // char a = 'a';
//     char buff = 'a';

//     // argv[0] = "wc";
//     // argv[1] = 0;

//     pipe(p1);
//     pipe(p2);

//     //int time = 1000;

//     if(fork() == 0) {
//         // close(0);
//         // dup(p1[0]);   //重定向管道输入端到标准输入
//         // close(p1[0]);
//         close(p1[1]);
//         close(p2[0]);


//         read(p1[0], &buff, 1);

//         printf("pid: %d recieve ping \r\n", getpid());
//         // write(1, &buff, sizeof(buff));
//         // printf("\r\n");

//         // close(1);
//         // dup(p2[1]);   //重定向管道输入端到标准输入
//         // close(p2[1]);
//         write(p2[1], &buff, 1);
//         // printf("pid: %d,write:", getpid());
//         // write(1, &buff, sizeof(buff));
//         // printf("\r\n");

//         // exit(0);
//     }
//     else
//     {
//         // close(1);
//         // dup(p1[1]);   //重定向管道输入端到标准输入
//         close(p1[0]);
//         close(p2[1]);
//         // close(p1[1]);
//         write(p1[1], &buff, 1);
//         // printf("pid: %d,write:", getpid());
//         // write(1, &buff, sizeof(buff));
//         // printf("\r\n");

//         // close(0);
//         // dup(p2[0]);   //重定向管道输入端到标准输入
//         // close(p2[0]);
//         read(p2[0], &buff, 1);

//         printf("pid: %d recieve pong \r\n", getpid());
//         // printf("pid: %d,read:", getpid());
//         // write(1, &buff, sizeof(buff));
//         // printf("\r\n");

//         wait(0);
//     }

//      exit(0);
// }