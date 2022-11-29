#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "defs.h"
#include "fcntl.h"
#include "fs.h"
#include "file.h"


struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

/*
error:
1.PTE:没有加PTE_U标签,导致用户空间无法使用映射的内存
2.PTE权限问题，在MAP_SHARED下，要依据文件权限赋予映射区域的读写权限
3.writefile的off由f->off决定，但是某些情况需要自己确定off,那么要直接调用writei
4.代码提前退出,返回-1时，一定要清空前面进行的操作
5.剩余1/3无法释放的问题,利用测试案例得出特殊情况
*/

inline int
trapmmap(uint64 va)
{
    struct proc* p = myproc();
    struct VMA* pvma = 0;
    uint64 pa;
    int perm = 0,r = 0;
    struct inode* ip;
    struct file* f;

    //1.确认异常出现地址是mmap引起,并定位该块vma

    int i = 0;
    for(;i < NVMA;i++){
      pvma = &p->vmas[i];
      if(  pvma->vma_ref == 1 &&
          (va >= pvma->vma_begin && va <= pvma->vma_end))
          {
            //printf("begin:%p,end:%p,va:%p \n",pvma->vma_begin,pvma->vma_end,va);
            break;
          }
    }

    if(i == NVMA){
      printf("can not find the vma \n");
      return -1;
    }

    //2.分配一块物理页,并读入4k文件到其中

    if((pa = (uint64)kalloc()) == 0)
      return -1;

    memset((void*)pa,0,PGSIZE);   //初始化

    f = pvma->vma_file;
    ip = f->ip;

    ilock(ip);

    //????文件偏移量???
    // if(n > (ip->size-f->off)){
    //   n = (ip->size-f->off);
    // }

    if((r = readi(ip, 0, pa, va-pvma->vma_begin, PGSIZE)) < 0){
      printf("read file failed \n");
      iunlock(ip);
      return -1;
    }
    iunlock(ip);

    f->off += r;

    //5.将读入文件的物理地址映射进入进程地址空间
    perm |= PTE_U;

    if(pvma->vma_perms & PROT_READ)
      perm |= PTE_R;

    if(pvma->vma_perms & PROT_WRITE)
      perm |= PTE_W;

    if(pvma->vma_perms & PROT_EXEC)
      perm |= PTE_X;

    
    if(mappages(p->pagetable,va,PGSIZE,pa,perm) < 0){
      printf("usertrap(): cannot map\n");
      kfree((void*)pa);
      p->killed = 1;
      return -1;
    }

    //1.写文件如何解决
    //2.不同的映射权限，如何保证shared情况下其他进程可以访问

    return 0;
}


//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();
  
  if(r_scause() == 8){
    // system call

    if(p->killed)
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sstatus &c registers,
    // so don't enable until done with those registers.
    intr_on();

    syscall();
  } else if((which_dev = devintr()) != 0){
    // ok
  } else if(r_scause() == 0x0d) {   //  Load page fault
      if(trapmmap(r_stval()) != 0) {
        printf("usertrap mmap process failed \n");
        p->killed = 1;
      }
  } 
  else {
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    p->killed = 1;
  }

  if(p->killed)
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2)
    yield();

  usertrapret();
}

//
// return to user space
//
void
usertrapret(void)
{
  struct proc *p = myproc();


  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to trampoline.S
  w_stvec(TRAMPOLINE + (uservec - trampoline));

  // set up trapframe values that uservec will need when
  // the process next re-enters the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 fn = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64,uint64))fn)(TRAPFRAME, satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    printf("scause %p\n", scause);
    printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if((scause & 0x8000000000000000L) &&
     (scause & 0xff) == 9){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000001L){
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if(cpuid() == 0){
      clockintr();
    }
    
    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}

