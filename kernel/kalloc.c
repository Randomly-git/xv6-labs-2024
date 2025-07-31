// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define BATCH_SIZE 8

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  char name[8];
} kmem[NCPU];

void
kinit()
{
  for(int i=0;i<NCPU;i++){
    snprintf(kmem[i].name,6,"kmem%d",i);
    initlock(&kmem[i].lock, kmem[i].name);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off();
  int cpu_id = cpuid();
  acquire(&kmem[cpu_id].lock);
  r->next = kmem[cpu_id].freelist;
  kmem[cpu_id].freelist = r;
  release(&kmem[cpu_id].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  struct run *tail;
  push_off();
  int cpu_id=cpuid();
  pop_off();
  acquire(&kmem[cpu_id].lock);
  r = kmem[cpu_id].freelist;
  if(r){
    kmem[cpu_id].freelist = r->next;
    release(&kmem[cpu_id].lock);
    }
  else{
    release(&kmem[cpu_id].lock);
    int i=(cpu_id+1)%NCPU;
    for(;i!=cpu_id;i=(i+1)%NCPU){
      //steal 2 or 3 everytime
      acquire(&kmem[i].lock);
      if (kmem[i].freelist)
      {
        // Steal multiple pages in one go
        r = kmem[i].freelist;
        tail = r;
        // count = 1;

        for(int count=1;count < BATCH_SIZE && tail->next;count++){
          tail = tail->next;
        }
        // Detach stolen pages from source CPU
        kmem[i].freelist = tail->next;
        release(&kmem[i].lock);
        tail->next = 0;

        // Add to local freelist
        acquire(&kmem[cpu_id].lock);
         // First page to return
        kmem[cpu_id].freelist = r->next;
        release(&kmem[cpu_id].lock);
        break;
      }
      release(&kmem[i].lock);
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
