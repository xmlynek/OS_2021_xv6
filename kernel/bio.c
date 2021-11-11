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

struct {
  struct spinlock lock;
  struct spinlock bucket_lock[NBUCKET];
  struct buf buf[NBUF];

  // Hash table
  struct buf head[NBUCKET];
} bcache;

void
move_to_bucket(struct buf *b, int destination_bucket)
{

    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[destination_bucket].next;
    b->prev = &bcache.head[destination_bucket];
    bcache.head[destination_bucket].next->prev = b;
    bcache.head[destination_bucket].next = b;
}

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKET; i++){
    initlock(&bcache.bucket_lock[i], "bcache.bucket");

  // Create linked lists of buffers
  bcache.head[i].prev = &bcache.head[i];
  bcache.head[i].next = &bcache.head[i];
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->blockno = 0;
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    b->ticks = ticks;
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int bucket = blockno % NBUCKET;
  acquire(&bcache.bucket_lock[bucket]);

  // Is the block already cached?
  for(b = bcache.head[bucket].next; b != &bcache.head[bucket]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket_lock[bucket]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  acquire(&bcache.lock);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  struct buf *min = 0;
  uint64 min_ticks = ~0;
  int min_bucket = 0;

  for(int i = 0; i < NBUCKET; i++){
    if(i != bucket)
      acquire(&bcache.bucket_lock[i]);
    int exists = 0;
    for(b = bcache.head[i].next; b != &bcache.head[i]; b = b->next){
      if(b->refcnt == 0 && b->ticks < min_ticks){
	if(min != 0){
	  min_bucket = min->blockno % NBUCKET;
	  if(min_bucket != i && min_bucket != bucket)
	    release(&bcache.bucket_lock[min_bucket]);
	}
	min_ticks = b->ticks;
	min = b;
	exists = 1;
      }
    }
    if(!exists)
      if(i != bucket)
        release(&bcache.bucket_lock[i]);
  }

  if(min == 0)
    panic("bget: no buffers");

  min_bucket = min->blockno % NBUCKET;
  min->dev = dev;
  min->blockno = blockno;
  min->valid = 0;
  min->refcnt = 1;
  if(min_bucket != bucket){
    move_to_bucket(min, bucket);
    release(&bcache.bucket_lock[min_bucket]);
  }
  release(&bcache.lock);
  release(&bcache.bucket_lock[bucket]);
  acquiresleep(&min->lock);
  return min;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
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

  int bucket = b->blockno % NBUCKET;
  acquire(&bcache.bucket_lock[bucket]);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->ticks = ticks;
  }
  
  release(&bcache.bucket_lock[bucket]);
}

void
bpin(struct buf *b) {
  int bucket = b->blockno % NBUCKET;
  acquire(&bcache.bucket_lock[bucket]);
  b->refcnt++;
  release(&bcache.bucket_lock[bucket]);
}

void
bunpin(struct buf *b) {
  int bucket = b->blockno % NBUCKET;
  acquire(&bcache.bucket_lock[bucket]);
  b->refcnt--;
  release(&bcache.bucket_lock[bucket]);
}


