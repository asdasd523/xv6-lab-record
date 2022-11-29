struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  //struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];

  uint timestamp;   // 时间戳
  // uchar key;        // 在buf数组中的位置
};

