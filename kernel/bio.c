// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"


#define NBUCKET 13
#undef NBUF
#define NBUF (NBUCKET * 5)


extern uint ticks;


struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.

  //struct buf head;
} bcache;

struct bucket{

  struct spinlock lock;
  struct buf head;        //利用head中的next将整个缓冲区连成链表

}hashtable[NBUCKET];


/*

1.为什么要使用hash table,有哪些好处
  (1)并行处理思想，将缓冲区分为NBUCKET个桶，那么就不用每次操作都对整个缓冲区都上锁,
  可以极大地提高处理效率
2.使用时间戳实现LRU算法有哪些好处
  (1)问题:使用链表LRU算法，在加入和取出block时都要对整个缓冲区加锁
  (2)

*/

void
binit(void)
{
  struct buf *b;
  uint i = 0,bucket;

  initlock(&bcache.lock, "bcache");

  for(int j = 0;j < NBUCKET;j++){

    initlock(&hashtable[j].lock,"hash");
    hashtable[j].head.next = 0;   //保证每个bucket链表的末尾都是0

  }
  // Create double linked list of buffers
  

  //利用hash table表达cache
  for(b = bcache.buf; b < (bcache.buf + NBUF); b++) {

    memset(b, 0, sizeof (struct buf));

    bucket = i % NBUCKET;   

    b->blockno = i;   

    acquire(&tickslock);
    b->timestamp = ticks;
    release(&tickslock);

    b->next = hashtable[bucket].head.next; //用hash table将cache串联
    hashtable[bucket].head.next = b;

    i++; 

    initsleeplock(&b->lock, "buffer");

  }

}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{

  //局部指针变量必须初始化，哪怕初始化为0
  struct buf *e = 0,*b = 0,*minblock = 0;   
  uint key = blockno % NBUCKET;
  uint mintime = (uint)MAXUINT;


  acquire(&hashtable[key].lock);

  //try to find target block(b) and min timestamp block(minblock)
  for(e = hashtable[key].head.next;e != 0;e = e->next) 
  {
    if(e->blockno == blockno && e->dev == dev)
        b = e;
  }


  //cached  
  if(b){
    b->refcnt++;
    release(&hashtable[key].lock);
    acquiresleep(&b->lock);
    return b;
  }
  //Not cached
  else{

    release(&hashtable[key].lock);

    //整个buf找最小的
    minblock = 0;
    mintime = (uint)MAXUINT;

    //bcache锁获取之后，那么bucket锁有什么存在的必要呢???
    //acquire(&bcache.lock);

    for(e = bcache.buf ; e < bcache.buf + NBUF; e++){
      if(mintime > e->timestamp && e->refcnt == 0){ 
        minblock = e;
        mintime  = e->timestamp;
      }
    }

    if(minblock != 0){

      uint minkey = minblock->blockno % NBUCKET;

      //acquire(&hashtable[minkey].lock);

      struct buf *buff = &hashtable[minkey].head;

      for(; buff->next != 0; buff = buff->next) {       

        if(buff->next == minblock){
          buff->next = minblock->next;  //删除
          break;
        } 

      }

      //release(&hashtable[minkey].lock);


      //acquire(&hashtable[key].lock);

      minblock->next = hashtable[key].head.next;
      hashtable[key].head.next = minblock;

      //release(&hashtable[key].lock);

      //release(&bcache.lock);

      goto finded;

    }
    
    panic("bget can not find");


  }

finded:

  acquire(&hashtable[key].lock);
  minblock->dev = dev;
  minblock->blockno = blockno;
  minblock->valid = 0;
  minblock->refcnt = 1;
  release(&hashtable[key].lock);

  acquiresleep(&minblock->lock);
  return minblock;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {         //如果缓存没有copy该block,那么重新分配一段缓存，并读入该block到该缓存中
    virtio_disk_rw(b, 0); 
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&hashtable[b->blockno % NBUCKET].lock);
  b->refcnt--;
  release(&hashtable[b->blockno % NBUCKET].lock);


  if (b->refcnt == 0) {

    acquire(&tickslock);
    b->timestamp = ticks;
    release(&tickslock);

  }

}

void
bpin(struct buf *b) {          //对每一块block的引用次数计数，+1
  acquire(&hashtable[b->blockno % NBUCKET].lock);
  b->refcnt++;
  release(&hashtable[b->blockno % NBUCKET].lock);
}

void
bunpin(struct buf *b) {       //对每一块block的引用次数计数，-1
  acquire(&hashtable[b->blockno % NBUCKET].lock);
  b->refcnt--;
  release(&hashtable[b->blockno % NBUCKET].lock);
}






