// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"


void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

//内核可用内存起始位置(做了对齐处理)
#define    kstart          PGROUNDUP((uint64)end)

//利用物理地址p求数组的下标数
#define    N(p)      (((PGROUNDDOWN((uint64)p)-(uint64)kstart) >> 12))

//用于存储引用值的内存段结束的位置
#define   kend          ((uint64)kstart+N(PHYSTOP))

#define   blocksz       (((uint64)PHYSTOP - kend) / NCPU)

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];



struct {
  //add
  struct spinlock reflock;  //维护计数数组的自旋锁
  char *paref;              //映射的用于计数的数组(起始位置kstart)
} counter;

int getblockNum(void* pa)
{
  int i = 0;
  char* p = (char*) kend;

  if((uint64)pa - kend < 0){
    panic("block num");
  }

  for(;p + blocksz <= (char*)PHYSTOP;p += blocksz){
    if((char*)pa >= p && (char*)pa <= p+blocksz)
      break;
    i++;
  }

  if(i > (NCPU-1)) panic("get block NUM");
  
  return i;
}

void
kinit()
{
  initlock(&counter.reflock,"reflock");
  counter.paref = (char*)kstart;    //paref映射的用于计数的数组(起始位置kstart)


  freerange((void*)kend, (void*)PHYSTOP);   //初始化空闲列表
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  int j = 0;

  p = (char*)PGROUNDUP((uint64)pa_start);

  for(int i = 0;i < NCPU;i++) {
    initlock(&kmem[i].lock, "kmem");
  }

  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){

    if((uint64)p > ((j+1)*blocksz + PGROUNDUP((uint64)pa_start)))
      if(j < 8)
        j++;

    acquire(&counter.reflock);
    *(counter.paref+N(p)) = (char)j;  //给该页赋分配的cpu的id
    release(&counter.reflock);

    kfree(p);

  }
}



// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  int n;

  //保证释放的物理内存是对齐的(4k)
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");


  acquire(&counter.reflock);
  n = *(counter.paref+N(pa));
  release(&counter.reflock);
  

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem[n].lock);
  r->next = kmem[n].freelist;
  kmem[n].freelist = r;
  release(&kmem[n].lock);
}




// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  //获取cpu id
  int id = (int)r_tp();

  acquire(&kmem[id].lock);
  r = kmem[id].freelist;
  if(r){
    kmem[id].freelist = r->next;
    release(&kmem[id].lock);
  }
  else{                                //当前cpu没有空余空间

    release(&kmem[id].lock);

    for(int i = 0;i < NCPU;i++){
      if(i == id) continue;

      acquire(&kmem[i].lock);

      if(kmem[i].freelist){

        r = kmem[i].freelist;
        kmem[i].freelist = r->next;

        acquire(&counter.reflock);
        *(counter.paref+N(r)) = id;   //给分配的页标上id
        release(&counter.reflock);

        release(&kmem[i].lock);
        break;
      }

      release(&kmem[i].lock);

    }

  }

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
  }

  return (void*)r;
}
