#include "kernel/types.h"
#include "user/user.h"


int main(int argc,char* argv[]){
    
    if(argc == 1){
        printf("error: please enter time number \r\n");
        exit(1);
    }

    int time = atoi(argv[2]);

    sleep(time);
    
    exit(0);   //退出并回收当前进程的资源

    return 0;
}