#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/fs.h"

#define nullptr (void*)0


void find_file(char* path,char* file){

    char buf[512], *p;
    struct dirent de;  //目录描述?
    struct stat st;
    int fd;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){       //通过文件描述符获取文件元状态(inode信息等)
        fprintf(2, "cannot stat %s\n", path);
        close(fd);
        return;
    }

    if(st.type != T_DIR){   //进入此函数的路径参数必须是目录
        close(fd);
        return;
    }

    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
        printf("find : path too long\n");
    }

    strcpy(buf,path);

    p = buf+strlen(buf);
    *p++ = '/';    //p指向'/'的填下一个路径名处

    while(read(fd, &de, sizeof(de)) == sizeof(de)){   //读取目录中的文件或子目录
        if(de.inum == 0) continue;
        memmove(p, de.name, DIRSIZ);                  //在循环中，依次获取目录中的文件名，直接覆盖上次赋给的文件名

        if(stat(buf, &st) < 0){                       //取该路径指示的文件的元数据
            printf("ls: cannot stat %s\n", buf);
            continue;
        }

        if(st.type == T_DIR){
            if(*p != '.' && *(p+1) != '.')            //除去.和..目录
                find_file(buf,file);                  //递归进入下一个目录寻找
        }
        else if(st.type == T_FILE || st.type == T_DEVICE){
            if(strcmp(p, file) == 0){
                printf("%s\n",buf);
            }
        }
    }
}

int main(int argc,char* argv[]){
    char* path,*file;

    if(argc < 3){
        printf("error with param \n");
        exit(1);
    }else{
        path = argv[1];                          //获取目录名
        file = argv[2];                          //获取需要查找的文件名

        if(path == nullptr || file == nullptr){
            printf("error with param \n");
            exit(1);
        }
    }

    find_file(path,file);
    exit(0);

    return 0;
}