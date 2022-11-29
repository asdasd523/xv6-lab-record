#include "kernel/types.h"
#include "user/user.h"


int main(int argc,char* argv[]){
    
    int pip[2];
    pipe(pip);    //利用一个管道双向传输


    if(fork() == 0){

        char son_sbuf = 'a';
        char son_rbuf;

        read(pip[0],&son_rbuf,sizeof(son_rbuf));

        if(son_rbuf == 'a')
            printf("%d: received ping\n",getpid());

        write(pip[1],&son_sbuf,sizeof(son_sbuf));

    }else{
        char parent_sbuf = 'a';
        char parent_rbuf;

        write(pip[1],&parent_sbuf,sizeof(parent_sbuf));

        read(pip[0],&parent_rbuf,sizeof(parent_rbuf));

        if(parent_rbuf == 'a')
            printf("%d: received pong\n",getpid());

        wait(0);
    }

    exit(0);

    return 0;
}