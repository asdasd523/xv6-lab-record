#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

//1.了解linux内核的sleep和wake up机制
//2.C与C++方式的条件变量与信号量的实现


static int nthread = 1;
static int round = 0;


struct barrier {
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread;      // Number of threads that have reached this round of the barrier
  int round;     // Barrier round
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
}

// static void 
// barrier()
// {
//   // YOUR CODE HERE
//   //
//   // Block until all threads have called barrier() and
//   // then increment bstate.round.
//   //
//   bstate.nthread++;         //每个线程加一次，并在barrier中判断是否全部线程已访问

//   if(nthread == bstate.nthread){   //最后一个线程经过
//     //激活所有等待着的线程
//     //printf("here 2\n");
//     pthread_cond_broadcast(&bstate.barrier_cond);
//     bstate.round++;
//     goto end;
//   }

//   //while(nthread != bstate.nthread){
//     //printf("here 1 \n");

//     //唤醒后，线程又会去竞争锁，拿到锁之后才会继续执行
//     //1.wait时，在cond_wait中的线程会持续release 锁
//     //2.唤醒并继续执行的条件:(1)被另一个线程唤醒 (2)能竞争到锁
//     pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex); //wait中一直在release锁
//  //}

//     //printf("here 3\n");


//   //pthread_mutex_unlock(&bstate.barrier_mutex);

//   end:
//     pthread_mutex_unlock(&bstate.barrier_mutex);


//     //pthread_mutex_lock(&bstate.barrier_mutex);
//     bstate.nthread--;
//     printf("%d \n",bstate.nthread);
//     //pthread_mutex_unlock(&bstate.barrier_mutex);
//     // while(bstate.nthread != 0)
//     // {
      
//     // }
//     // printf("waiting end %d\n",bstate.nthread);
// }

static void
barrier()
{
  // YOUR CODE HERE
  //
  // Block until all threads have called barrier() and
  // then increment bstate.round.

  pthread_mutex_lock(&bstate.barrier_mutex);
  bstate.nthread++;    //每个线程加一次，并在barrier中判断是否全部线程已访问

  if(bstate.nthread == nthread) {  //最后一个经过的线程
    //利用最后一个经过的线程唤醒全部线程
    bstate.round++;
    bstate.nthread = 0;
    pthread_cond_broadcast(&bstate.barrier_cond); 
  }else
    pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);  //等待唤醒

  pthread_mutex_unlock(&bstate.barrier_mutex);

}



static void *
thread(void *xa)
{
  long n = (long) xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++) {
    
    int t = bstate.round;
    assert (i == t);
    barrier();
    usleep(random() % 100);
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}
