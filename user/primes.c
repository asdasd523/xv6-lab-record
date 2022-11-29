#include "kernel/types.h"
#include "user/user.h"

void primes(int pip[2]){

    int pip1[2];
    int prime;

    close(pip[1]);   
    //递归终止条件
    if(read(pip[0],&prime,sizeof(prime)) == 0){
        close(pip[0]);
        exit(0);
    }
    else{
        pipe(pip1);
        if(fork() == 0){
            close(pip[0]);
            primes(pip1);
        }
        else{
            printf("prime %d\n",prime);
            int n;
            while(1){
                if(read(pip[0],&n,sizeof(n)) == 0){
                    close(pip[0]);
                    break;
                }
                else{
                    if(n % prime != 0)
                        write(pip1[1],&n,sizeof(n));
                }
            }
            close(pip1[0]);
            close(pip1[1]);
            wait(0);
        }
        exit(0);
    }

}
int main(int argc,char* argv[]){

    //父子进程会copy管道的描述符，不及时关闭会造成描述符使用完?
    int pip[2];

    pipe(pip);

    if(fork() == 0){
        primes(pip);
    }else{
        close(pip[0]);   //父进程关闭读端不会对子进程造成影响,父子进程的管道描述符是共享还是拷贝

        for(int i = 2;i <= 35;i++)
            write(pip[1],&i,sizeof(i));

        close(pip[1]);   

        wait(0);
    }
    exit(0);   //退出

}