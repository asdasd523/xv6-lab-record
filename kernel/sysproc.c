#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

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
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
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

// pte_t *
// walk(pagetable_t pagetable, uint64 va, int alloc)
// {
//   if(va >= MAXVA)
//     panic("walk");

//   for(int level = 2; level > 0; level--) {
//     pte_t *pte = &pagetable[PX(level, va)];   //PX : 0~511,PX:根据Level+12，决定到哪一段找PTE
//     if(*pte & PTE_V) {
//       pagetable = (pagetable_t)PTE2PA(*pte);  //第一级页表走完，pagetable就装了第二级，然后第三级
//     } else {
//       if(!alloc || (pagetable = (pde_t*)kalloc()) == 0)
//         return 0;
//       memset(pagetable, 0, PGSIZE);
//       *pte = PA2PTE(pagetable) | PTE_V;
//     }
//   }
//   return &pagetable[PX(0, va)];               //三级页表走完，查到0级页表
// }
#ifdef LAB_PGTBL

int
sys_pgaccess(void)
{

  //1. argchar的作用要弄清楚
  //2. 虚拟地址加一页是什么
      

  // lab pgtbl: your code here.
  uint64 va;
  uint64 addr,num = 0;
  int n;

  argaddr(0,&va);//the starting virtual address of the first user page to check
  argint (1,&n);    //the number of pages to check.
  argaddr(2,&addr); //takes a user address to a buffer to store the results into a bitmask

  struct proc* p = myproc();
  pagetable_t pagetable = p->pagetable;  //获取当前进程的页表
  pte_t* pte;

  if(va >= (uint64)MAXVA){
    printf("sys_pgaccess va error\n");
    return -1;
  }

  for(int i = 0;i < n;i++){
    if(i > 0)
     va += (uint64)PGSIZE;

    if((pte = walk(pagetable, va, 0)) == 0){
      printf("pgaccess walk error\n");
      return -1;
    }

    if(*pte & PTE_A){
       num |= (1<<i);
      *pte &= ~PTE_A;
    }
  }

  if(copyout(pagetable,addr,(char*)&num,sizeof(num)) == -1)
    return -1;


  return 0;
}
#endif

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
