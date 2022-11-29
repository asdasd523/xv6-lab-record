#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)           //申请一段虚拟内存(即用户空间内存)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;   //获取进程空间大小
  if(growproc(n) < 0)    //增长n个空间的大小
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_trace(void)
{
  int mask;
  argint(0, &mask);           //获取参数

  struct proc* p = myproc();  //获取执行此函数的进程PCB
  if(!p)
    return -1;

  p->mask = mask;             //拷贝进此进程的PCB中，通过PCB将mask值传到syscall函数中

  return 0;
}

uint64
sys_sysinfo(void)
{
  uint64 addr;
  struct sysinfo ifo;
  struct proc* p = myproc();
  argaddr(0, &addr);   //接收用户空间的虚拟地址

  ifo.freemem = getfreemem();
  ifo.nproc   = get_procnum();

  if(copyout(p->pagetable,addr,(char*)&ifo,sizeof(ifo)) < 0)
    return -1;

  return 0;
}