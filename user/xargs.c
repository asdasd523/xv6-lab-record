#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"


int main(int argc,char* argv[]){

    char rbuf[MAXARG],*p = rbuf;
    int n = 0;
    char* cmd[MAXARG];

    for(int i = 1;i < argc;i++){
        cmd[i-1] = argv[i];
    }
    int s = argc-1;

    //1.在管道中执行,fd=0指向管道的读端
    //2.exec的参数列表中不允许有换行符
    //3.为什么从管道中读，'/n'会在第一次读不出来
    while((n += read(0,p,MAXARG)) > 0){    //第二次读，覆盖了./b的第一位.,换成了'\n'
        p = rbuf + n;                 //指向第一次读完的末尾
        if(*(p-1) != '\n')           //没读到'\n'继续读
            continue; 
        else{
            *--p = '\0';              //把'\n'换成'\0'
            cmd[s] = malloc(n-1);
            memmove(cmd[s],rbuf,n-1); //放入命令数组
            s++;

            memset(rbuf,0,MAXARG);    //清空接收缓冲，继续读
            p = rbuf;
            n = 0;
        }
    }

    if(fork() == 0)
        exec(argv[1],cmd);
    else
        wait(0);
    

    exit(0);

    return 0;
}